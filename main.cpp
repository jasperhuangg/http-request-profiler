#include <iostream>
#include <vector>
#include <limits.h>
#include <algorithm>
#include <numeric>
#include <getopt.h>
#include <string.h>

#include "get.h"

static int help_flag = 0;
static int profile_flag = 0;
static int url_flag = 0;

std::string parseResponseContent(std::string response) {

    std::string str(response);
    int CLRFIdx = str.find("\r\n\r\n");
    std::string content = str.substr(CLRFIdx + 4);

    return content;
}

int parseResponseStatus(std::string response) {

    int startIdx = response.find("HTTP/1.1 ") + 9;
    return stoi(response.substr(startIdx, 3));
}

void getArgs(int argc, char** argv, char *&url, int &numRequests) {
    int c, digit_optind;
    while (1)
    {
        struct option longopts[] = {
            {"url", required_argument, NULL, 'u'},
            {"profile", required_argument, NULL, 'p'},
            {"help", no_argument, &help_flag, 1},
            {0, 0, 0, 0}
        };
        int optindex = 0;

        c = getopt_long(argc, argv, "", longopts, &optindex);
        if (c == -1) break;

        if (c == 'u') {
            url = optarg;
            url_flag = 1;
        }
        if (c == 'p') {
            numRequests = atoi(optarg);
            profile_flag = 1;
        }
    }
}

// TODO: https://www.man7.org/linux/man-pages/man3/getopt.3.html
int main(int argc, char** argv) {

    std::cout << std::endl;

    // ---- Parse arguments
    char *url;
    int numRequests = -1;
    getArgs(argc, argv, url, numRequests);

    if (help_flag) {
        std::cout << "Usage: ./profile --url <url> --profile <numRequests> --help" << std::endl;
        std::cout << "    > url: the url to make requests to" << std::endl;
        std::cout << "    > numRequests: the number of requests to make when profiling (optional)." << std::endl << std::endl;
        return 1;
    }

    if (!url_flag) {
        std::cout << "ERROR, must provide URL." << std::endl << std::endl;
        return 0;
    }

    std::vector<int> times;
    std::string nonSuccessCodes = "";
    int smallestResponseSize = INT_MAX, largestResponseSize = 0;
    int succeeded = 0;
    std::string urlStr(url);
    
    if (!profile_flag) {
        // ---- Make a request and print the content of the response
        std::pair<std::string, long> responsePair = get(urlStr);
        int status = parseResponseStatus(responsePair.first);
        std::string content = parseResponseContent(responsePair.first);
        if (status == 200)
            std::cout << content << std::endl << std::endl;
        else
            std::cout << "status " << status << std::endl << std::endl;
        return 1;
    }

    if (numRequests <= 0) {
        std::cout << "ERROR, numRequests must be greater than 0" << std::endl << std::endl;
        return 0;
    }

    for (int i = 0; i < numRequests; i++) {
        std::pair<std::string, long> responsePair = get(urlStr);
        std::string response = responsePair.first;

        // ---- Get the size of the response
        int responseBytes = response.length();
        if (responseBytes < smallestResponseSize)
            smallestResponseSize = responseBytes;
        if (responseBytes > largestResponseSize)
            largestResponseSize = responseBytes;

        // ---- Add the time taken for the request
        long timeTaken = responsePair.second;
        times.push_back(timeTaken);

        // ---- Update values depending on response status
        int status = parseResponseStatus(response);
        if (status == 200) {
            succeeded++;
        }
        else {
            nonSuccessCodes += std::to_string(status) + " ";
            std::cout << ">>>> failed"
                      << "\n\n";
        }
    }

    // ---- Find the fastest, slowest, median, and mean times
    std::sort(times.begin(), times.end());
    int fastestTime = times.at(0);
    int slowestTime = times.at(times.size() - 1);
    int medianTime = times.size() % 2 ? times.at(times.size() / 2) : (times.at(times.size() / 2) + times.at(times.size() / 2 + 1)) / 2;
    double meanTime = (double)std::accumulate(times.begin(), times.end(), 0) / (double)times.size();

    // ---- Print profiling results
    std::cout << "================================================================================\nRESULTS:\n\n";
    std::cout << ">>>> Fastest time: " << fastestTime << "ms" << std::endl;
    std::cout << ">>>> Slowest time: " << slowestTime << "ms" << std::endl;
    std::cout << ">>>> Mean time: " << meanTime << "ms" << std::endl;
    std::cout << ">>>> Median time: " << medianTime << "ms" << std::endl;
    std::cout << ">>>> Percentage successful requests: " << (double)succeeded / (double)numRequests * 100.0 << "%" << std::endl;
    std::cout << ">>>> Unsuccessful error codes: " << (nonSuccessCodes == "" ? "N/A" : nonSuccessCodes) << std::endl;
    std::cout << ">>>> Smallest response size: " << smallestResponseSize << " bytes" << std::endl;
    std::cout << ">>>> Largest response size: " << largestResponseSize << " bytes"<< std::endl;

    return 1;
}
