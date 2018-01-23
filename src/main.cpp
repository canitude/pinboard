/**
 * Copyright (c) 2017-2018
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <future>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <altcoin/network.hpp>
#include <bitcoin/bitcoin/math/hash.hpp>
#include <bitcoin/bitcoin/message/messages.hpp>
#include <bitcoin/bitcoin/message/version.hpp>

#include "lite_node.hpp"
#include "chain_listener.hpp"
#include "pinboard.hpp"
#include "config.hpp"
#include "miner.hpp"

#define LOG_MAIN "main"

using namespace std;
using namespace boost::program_options;
using namespace bc::network;
using namespace bc::chain;
using namespace bc::node;
using namespace bc::message;

struct parameters
{
    bool action_print_and_exit = false;
    bool action_submit_and_exit = false;
    bool dont_guess_external_ip = false;

    string manually_set_ip;

    string new_message_body;  // for "submit" action
};

bc::network::settings get_network_settings(bool mainnet);
parameters parse_command_line(int argc, char* argv[], bc::network::settings& settings);
lite_header get_last_checkpoint();
void prepare_settings(bc::network::settings &settings, parameters &param);

int main(int argc, char* argv[])
{
    try
    {
        auto settings = get_network_settings(true); // Litecoin P2P network settings
        auto param = parse_command_line(argc, argv, settings);
        auto last_known_checkpoint = get_last_checkpoint(); // Hard-coded checkpoint
        prepare_settings(settings, param);

        // Create everything
        auto mb = make_shared<message_broadcaster>();
        auto ch = make_shared<chain_sync_state>(mb, last_known_checkpoint);
        auto pb = make_shared<pinboard>(mb, ch, MIN_TARGET);
        pb->start([](const bc::code&){});
        auto ln = make_shared<lite_node>(settings, ch, pb);
        mb->link_to_node(ln);
        ln->set_top_block(bc::config::checkpoint(last_known_checkpoint.hash(), last_known_checkpoint.validation.height));

        // Run networking
        ln->start([&ln, &ch, &mb, &pb, &param](const bc::code &ec) {
            LOG_INFO(LOG_MAIN) << "lite_node::start got ec == " << ec;

            ln->run([&ln, &ch, &mb, &pb, &param](const bc::code &ec) {
                LOG_INFO(LOG_MAIN) << "lite_node::run got ec == " << ec;

                if (param.action_print_and_exit)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(30));
                    LOG_INFO(LOG_MAIN) << endl << pb->to_string();

                    if (!param.action_submit_and_exit)
                    {
                        if (!ln->stop())
                        {
                            LOG_WARNING(LOG_MAIN) << "lite_node::stop returned false";
                            exit(EXIT_FAILURE);
                        }
                        LOG_INFO(LOG_MAIN) << "Shutdown complete.";
                        exit(EXIT_SUCCESS);
                    }
                }

                if (param.action_submit_and_exit)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    while (!ch->is_synchronized())
                    {
                        LOG_INFO(LOG_MAIN) << "Waiting for blockchain sync ... ";
                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                    LOG_INFO(LOG_MAIN) << "Starting miner ... ";

                    auto msg = make_shared<object_payload>(param.new_message_body);
                    auto mn = make_shared<miner<default_pow>>(msg, ch);

                    const bc::uint256_t target = MIN_TARGET;
                    mn->start_mining(target, [&ln, &ch, &mb](const bc::code &ec, object_payload::ptr obj)
                    {
                        LOG_INFO(LOG_MAIN) << "miner::start_mining got ec == " << ec;
                        LOG_INFO(LOG_MAIN) << "nonce = " << obj->get_nonce() << " work_done = " << obj->get_work_done();

                        bc::data_chunk data = obj->to_data(0);
                        LOG_INFO(LOG_MAIN) << "Obj dump: " << bc::encode_base16(data);
                        LOG_INFO(LOG_MAIN) << obj->to_string();

                        LOG_INFO(LOG_MAIN) << "Broadcasting ... ";
                        auto new_object = make_shared<object>(*(obj.get()));

                        mb->broadcast_to_pb(*(new_object.get()),
                        [](const bc::code &ec, typename channel<message_subscriber_ex>::ptr channel)
                        {
                            LOG_INFO(LOG_MAIN) << "Broadcasted to [" << channel->authority() << "] with code " << ec;
                        },
                        [&ln](const bc::code &ec)
                        {
                            if (ec == bc::error::success)
                                LOG_INFO(LOG_MAIN) << "Broadcasting: success. Shutting down...";
                            else
                                LOG_WARNING(LOG_MAIN) << "Broadcasting: failed with code " << ec << ". Shutting down...";

                            if (!ln->stop())
                            {
                                LOG_WARNING(LOG_MAIN) << "lite_node::stop returned false";
                                exit(EXIT_FAILURE);
                            }
                            LOG_INFO(LOG_MAIN) << "Shutdown complete.";
                            exit(EXIT_SUCCESS);
                        });
                    });
                }
            });
        });

        // Construct a signal set registered for process termination.
        boost::asio::io_service io_service;
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

        // Start an asynchronous wait for one of the signals to occur.
        signals.async_wait([&ln, &pb](const boost::system::error_code &error, int signal_number) {
            cout << "Signal " << signal_number << " is caught. Shutting down." << endl;

            if (!ln->stop())
                LOG_WARNING(LOG_MAIN) << "lite_node::stop returned false";
            else
                LOG_INFO(LOG_MAIN) << "Shutdown complete.";

            pb->stop();
        });

        io_service.run();
    }
    catch (const error &ex)
    {
        cerr << ex.what() << '\n';
    }

    return 0;
}

string guess_my_ip_with_google_dns();

void print_help_message(const options_description& commands)
{
    cout << "Usage:" << endl;
    cout << "  pinboard\t\t\tStart node" << endl;
    cout << "  pinboard --print\t\tStart node, synchronize with network," << endl;
    cout << "\t\t\t\tprint all messages from pinboard and exit" << endl;
    cout << endl;
    cout << "  echo \"Hello world!\" | pinboard --submit" << endl;
    cout << "\t\t\t\tStart node, synchronize with network," << endl;
    cout << "\t\t\t\tgenerate PoW for message \"Hello world!\"" << endl;
    cout << "\t\t\t\tsumbit it to other nodes and exit" << endl;
    cout << endl << "  Use Ctrl-C to stop node." << endl << endl;
    cout << commands << endl;
}

parameters parse_command_line(int argc, char* argv[], bc::network::settings& settings)
{
    parameters param;

    options_description commands{"Options"};
    commands.add_options()
            ("help,h", "This help message")
            ("print,p", "Print all messages from pinboard and exit")
            ("submit,s", "Submit message from STDIN and exit")
            ("inbound-port,i",
             value<uint16_t>(&settings.inbound_port)->implicit_value(29333),
             "Inbound port for p2p communication")
            ("max-inbound", value<uint32_t>(), "Maintain at most <arg> inbound p2p connections")
            ("max-outbound", value<uint32_t>(), "Maintain at most <arg> outbound p2p connections")
            ("max-addresses", value<uint32_t>(), "Store at most <arg> peer addresses")
            ("connect-to", value<vector<string>>()->multitoken()->composing(), "List of peers to connect to" )
            ("set-ip", value<string>(), "Store at most <arg> peer addresses")
            ("dont-use-seeds", "Don't ask Litecoin seeds for peer addresses" )
            ("dont-guess-ip", "Don't guess external ip" );

    command_line_parser parser{argc, argv};
    parser.options(commands);
    parsed_options parsed_options = parser.run();

    variables_map vm;
    store(parsed_options, vm);
    notify(vm);

    if (vm.count("help"))
    {
        print_help_message(commands);
        exit(EXIT_SUCCESS);
    }

    if (vm.count("print"))
    {
        cout << "Print command selected" << endl;
        param.action_print_and_exit = true;
    }

    if (vm.count("submit"))
    {
        if (isatty(0)) {
            cerr << "Error: --submit command used but there is no message in STDIN :(" << endl << endl;
            print_help_message(commands);
            exit(EXIT_FAILURE);
        }

        string line;
        while (getline(cin, line)) {
            if (param.new_message_body.size() > 0)
                param.new_message_body += '\n';
            param.new_message_body += line;
        }

        if (param.new_message_body.size() < 1) {
            cerr << "Error: --submit command used but the message is empty :(" << endl << endl;
            print_help_message(commands);
            exit(EXIT_FAILURE);
        }

        cout << "Going to submit message [" << param.new_message_body << "] and exit." << endl;

        param.action_submit_and_exit = true;
    }

    if (vm.count("connect-to"))
    {
        for (auto item : vm["connect-to"].as<std::vector<std::string>>())
            settings.peers.push_back(bc::config::endpoint(item));
    }

    if (vm.count("max-inbound"))
    {
        settings.inbound_connections = vm["max-inbound"].as<uint32_t>();
    }

    if (vm.count("max-outbound"))
    {
        settings.outbound_connections = vm["max-outbound"].as<uint32_t>();
    }

    if (vm.count("max-addresses"))
    {
        settings.host_pool_capacity = vm["max-addresses"].as<uint32_t>();
    }

    if (vm.count("dont-guess-ip"))
    {
        param.dont_guess_external_ip = true;
    }

    if (vm.count("dont-use-seeds"))
    {
        settings.seeds.clear();
    }

    if (vm.count("set-ip"))
    {
        param.manually_set_ip = vm["set-ip"].as<std::string>();
    }

    return param;
}

bc::network::settings get_network_settings(bool mainnet)
{
    bc::network::settings s;

    if (mainnet) {
        s.identifier = 0xDBB6C0FB;
        s.inbound_port = 29333;
    } else {
        s.identifier = 0xFDD2C8F1;
        s.inbound_port = 19335;
    }

    s.services = bc::message::version::service::node_network;
    s.services |= (1u << PINBOARD_SERVICE_BIT);

    s.manual_attempt_limit = 0;
    s.connect_batch_size = 1;
    s.inbound_connections = 16;
    s.outbound_connections = 16;
    s.host_pool_capacity = 256000;
    s.hosts_file = "hosts.txt";

    s.seeds.push_back(bc::config::endpoint("seed-a.litecoin.loshan.co.uk:9333"));
    s.seeds.push_back(bc::config::endpoint("dnsseed.thrasher.io:9333"));
    s.seeds.push_back(bc::config::endpoint("dnsseed.litecointools.com:9333"));
    s.seeds.push_back(bc::config::endpoint("dnsseed.litecoinpool.org:9333"));

    s.verbose = true;

    return s;
}

void prepare_settings(bc::network::settings &settings, parameters &param)
{
    if (settings.inbound_connections > 0)
    {
        if (param.manually_set_ip.size() > 0)
        {
            settings.self = bc::config::authority(param.manually_set_ip, settings.inbound_port);
        }
        else if (!param.dont_guess_external_ip)
        {
            string ip = guess_my_ip_with_google_dns();
            if (ip.size() > 0)
                settings.self = bc::config::authority(ip, settings.inbound_port);
            else
            {
                LOG_ERROR(LOG_MAIN) << "Failed to guess external ip with Google DNS. Turning off inbound connections.";
                settings.inbound_connections = 0;
            }
        }
    }

}

lite_header get_last_checkpoint()
{
    bc::hash_digest prev_hash(bc::null_hash);
    bc::hash_digest merkle_root(bc::null_hash);
    bc::hash_digest expected_header_hash(bc::null_hash);

    if (!bc::decode_hash(prev_hash, "d0a2824855062497a4b03c89b06def42abcb45158c406713cf219e5b4055a426")
        || !bc::decode_hash(merkle_root, "e97314257cbd625676411a9c295861256c3932bae95312a0672d99711daf40d1")
        || !bc::decode_hash(expected_header_hash, "2dd9a6d0d30ded8925c303b8228713e72c345e0e3aed488897643d6d35b9d6ee"))
    {
        LOG_ERROR(LOG_MAIN) << "Can't decode hashes.";
        throw;
    }

    lite_header h(536870912,       /* version */
                  prev_hash,
                  merkle_root,
                  1514572031,      /* timestamp */
                  0x1a04865f,      /* bits */
                  2046883480       /* nonce */ );

    h.validation.height = 1341188; /* height */

    LOG_INFO(LOG_MAIN) << "Checkpoint hash: " <<  bc::encode_base16(h.hash());

    if ((h.hash() != expected_header_hash))
    {
        LOG_ERROR(LOG_MAIN) << "Wrong checkpoint.";
        throw;
    }

    return h;
}
