#pragma once

#include <cstddef>

namespace nargparse {

const std::size_t kMaxArgLen = 128;

enum ArgType {
    kTypeInt,
    kTypeFloat,
    kTypeString,
    kTypeBool
};

enum NargsType {
    kNargsOptional,
    kNargsRequired,
    kNargsZeroOrMore,
    kNargsOneOrMore
};

typedef bool (*IntValidator)(const int&);
typedef bool (*FloatValidator)(const float&);
typedef bool (*StringValidator)(const char* const&);

struct Parser;
typedef Parser* ArgumentParser;

ArgumentParser CreateParser(const char* name);
ArgumentParser CreateParser(const char* name, std::size_t max_arg_len);
void FreeParser(ArgumentParser parser);

void AddHelp(ArgumentParser parser);
void PrintHelp(ArgumentParser parser);

void AddFlag(ArgumentParser parser,
             const char* short_name,
             const char* long_name,
             bool* flag,
             const char* description,
             bool default_value = false);

void AddArgument(ArgumentParser parser,
                 int* value,
                 const char* name,
                 NargsType nargs = kNargsRequired,
                 IntValidator validator = nullptr,
                 const char* error_msg = nullptr);
void AddArgument(ArgumentParser parser,
                 float* value,
                 const char* name,
                 NargsType nargs = kNargsRequired,
                 FloatValidator validator = nullptr,
                 const char* error_msg = nullptr);
void AddArgument(ArgumentParser parser,
                 char (*value)[kMaxArgLen],
                 const char* name,
                 NargsType nargs = kNargsRequired,
                 StringValidator validator = nullptr,
                 const char* error_msg = nullptr);

void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 int* value,
                 const char* name,
                 NargsType nargs = kNargsOptional,
                 IntValidator validator = nullptr,
                 const char* error_msg = nullptr);
void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 float* value,
                 const char* name,
                 NargsType nargs = kNargsOptional,
                 FloatValidator validator = nullptr,
                 const char* error_msg = nullptr);
void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 char (*value)[kMaxArgLen],
                 const char* name,
                 NargsType nargs = kNargsOptional,
                 StringValidator validator = nullptr,
                 const char* error_msg = nullptr);

bool Parse(ArgumentParser parser, int argc, const char* argv[]);
bool Parse(ArgumentParser parser, int argc, char* argv[]);

int GetRepeatedCount(ArgumentParser parser, const char* name);
bool GetRepeated(ArgumentParser parser, const char* name, int index, int* value);
bool GetRepeated(ArgumentParser parser, const char* name, int index, float* value);
bool GetRepeated(ArgumentParser parser, const char* name, int index, const char** value);

}  // namespace nargparse
