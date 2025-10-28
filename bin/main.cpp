#include <iostream>
#include <lib/argparser.h>

struct Options {
    bool sum = false;
    bool mult = false;
};

bool IsEven(const int& value) {
    return value % 2 == 0;
}

int main(int argc, char** argv) {
    Options options;
    int value;
    std::vector<int> values;

    nargparse::ArgumentParser parser = nargparse::CreateParser("labwork3");

    nargparse::AddFlag(parser, "-s", "--sum", &options.sum, "Summarise");
    nargparse::AddFlag(parser, "-m", "--mult", &options.mult, "Multiply");
    nargparse::AddArgument(parser, &value, "values", nargparse::kNargsZeroOrMore, IsEven, "only even numbers");
    nargparse::AddHelp(parser);

    if(!nargparse::Parse(parser, argc, argv)) {
        nargparse::FreeParser(parser);
        nargparse::PrintHelp(parser);
        return 1;
    }


    int count = nargparse::GetRepeatedCount(parser, "values");
    for (int i = 0; i < nargparse::GetRepeatedCount(parser, "values"); ++i) {
        if (nargparse::GetRepeated(parser, "values", i, &value)) {
            values.push_back(value);
        }
    }

    nargparse::FreeParser(parser);

    int result = 0;
    if(options.sum) {
        for(int i = 0; i < values.size(); ++i) {
            result += values[i];
        }
    } else if(options.mult) {
        result = 1;
        for(int i = 0; i < values.size(); ++i) {
            result *= values[i];
        }
    }

    std::cout << "Result: " << result << std::endl;

    return 0;
}