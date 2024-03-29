/*
    A3Graphite.cpp

    Crude Aram 3 plugin to submit metrics in graphite line
    protocol over UDP to the target host

    "A3Graphite" callExtension "some.graphite.path|2.2"

*/

#include <algorithm>    // for std::remove_if
#include <ctime>
#include <string>
#include <boost/lexical_cast.hpp>

#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

bool graphite_sock_init = false;
struct sockaddr_in graphite_si;
SOCKET graphite_sock;

bool invalidMetricChar(char c) {
    return !(
        (c >= 48 && c <= 57)              // Numeric
        || (c >= 65 && c <= 90)           // A-Z
        || (c >= 97 && c <= 122)          // a-z
        || c == 32 || c == 95 || c == 46  // Space + Underscore + Period
        );
}

void cleanMetricName(std::string & str) {
    str.erase(remove_if(str.begin(), str.end(), invalidMetricChar), str.end());
    std::replace(str.begin(), str.end(), ' ', '_');
}

const char * process(std::string input) {

    size_t pos = input.find("|");
    if (pos != std::string::npos) {

        std::string metric = std::string(input.substr(0, pos));
        std::string value = input.substr(pos + 1, input.size());

        // Validate metric
        cleanMetricName(metric);

        if (metric == "") {
            return "invalid metric name";
        }
        else {

            std::string line;
            line.append(metric);

            // ascii alphanumerics, underscore and periods
            line.append(" ");

            try {
                boost::lexical_cast<double>(value);
            }
            catch (const boost::bad_lexical_cast) {
                return "invalid metric value";
            }

            line.append(value);

            line.append(" ");
            line.append(std::to_string(time(nullptr)));

            line.append("\n");

            // send it via socket
            const char * c_msg = line.c_str();

            sendto(graphite_sock, c_msg, (int)strlen(c_msg), 0, (struct sockaddr *) &graphite_si, sizeof(graphite_si));
            return c_msg;
        }
    }
    return "malformed, could not find separator";
}


extern "C"
{
    __declspec (dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
    --outputSize;

    const char *ret = "\0";
    struct addrinfo hints, *result;

    if (!graphite_sock_init) {

        if ((graphite_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
            ret = std::to_string(WSAGetLastError()).c_str(); // "failed to open socket";
        } else {

            char config_host[80] = "";
            char config_port[10] = "";

            const char* hostname = "localhost";
            int port = 2003;

            LPSTR parameter = GetCommandLineA();
            size_t lengh  = strlen(parameter);

            for (int i = 0; i<lengh; i++) {
                char *xff = (&parameter[i]);
                if (strncmp("-A3GraphiteHost", xff, sizeof("-A3GraphiteHost") - 1) == 0) {
                    sscanf_s(xff + sizeof("-A3GraphiteHost"), "%s", config_host, (unsigned)_countof(config_host));
                } else if (strncmp("-A3GraphitePort", xff, sizeof("-A3GraphitePort") - 1) == 0) {
                    sscanf_s(xff + sizeof("-A3GraphitePort"), "%s", config_port,  (unsigned)_countof(config_port));
                }
            }

            std::string host_result = std::string(config_host);
            if (!host_result.empty()) {
                if (host_result.back() == '"') {
                    hostname = host_result.substr(0, host_result.size() - 1).c_str();
                } else {
                    hostname = host_result.c_str();
                }
            }

            std::string port_result = std::string(config_port);
            if (!port_result.empty()) {
                if (port_result.back() == '"') {
                    port = atoi(port_result.substr(0, port_result.size() - 1).c_str());
                } else {
                    port = atoi(port_result.c_str());
                }
            }

            // Lookup IPv4 address
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;

            int error = getaddrinfo(hostname, NULL, &hints, &result);
            if (error != 0) {
                strncpy_s(output, outputSize, "host lookup failed", outputSize - 1);
                return;
            }

            // We pick the first record and use that
            memset((char *)&graphite_si, 0, sizeof(graphite_si));
            graphite_si.sin_family = AF_INET;
            graphite_si.sin_port = htons(port);
            graphite_si.sin_addr = ((struct sockaddr_in *) result->ai_addr)->sin_addr;

            graphite_sock_init = true;
        }
    }

    if (strlen(ret) != 0) {
        strncpy_s(output, outputSize, ret, outputSize-1);
        return;
    }

    std::string input = std::string(function);
    strncpy_s(output, outputSize, process(input), outputSize-1);
    return;
}
