#include <iostream>

int main(int argc, char* argv[]){
    std::cout << "PGW client " << '\n';
    if(argc > 1){
        std::cout << "IMSI: " << argv[1] << '\n';
    }
    else {
        std::cout << "IMSI client" << '\n';
    }

    return 0;
}