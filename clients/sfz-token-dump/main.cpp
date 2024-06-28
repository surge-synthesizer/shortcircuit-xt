#include <iostream>
#include "sample/sfz_support/sfz_parse.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " (sfzfile)" << std::endl;
        return 1;
    }

    auto f = fs::path{argv[1]};
    scxt::sfz_support::SFZParser parser;
    auto res = parser.parse(f);

    std::cout << "<sfzdump file=\"" << f.u8string() << "\">" << std::endl;

    for (const auto &[header, opcodes] : res)
    {
        std::cout << "  <header type=\"" << header.name << "\">" << std::endl;
        for (const auto &[key, value] : opcodes)
        {
            std::cout << "    <opcode key=\"" << key << "\" value=\"" << value << "\"/>"
                      << std::endl;
        }
        std::cout << "  </header>" << std::endl;
    }

    std::cout << "</sfzdump>" << std::endl;

    return 0;
}
