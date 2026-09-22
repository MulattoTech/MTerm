// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include <QCoreApplication>
#include <QThread>
#include <iostream>
#include <string>
int main(int argc,char **argv){
 QCoreApplication app(argc,argv); const auto args=app.arguments();
 if(args.contains("--wait")){QThread::msleep(5000);return 0;}
 if(args.contains("--flood")){std::cout<<std::string(600000,'x')<<std::flush;return 0;}
 if(args.contains("--stdin")){std::string s;std::getline(std::cin,s);std::cout<<s<<std::flush;return 0;}
 if(args.contains("--fail")){std::cerr<<"deliberate"<<std::flush;return 7;}
 std::cout<<"MTERM_CHILD_OK\n"<<std::flush;return 0;
}
