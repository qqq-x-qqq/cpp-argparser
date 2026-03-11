#include <gtest/gtest.h>
#include <lib/argparser.h>
#include <cctype>
#include <cstring>


static const size_t kMaxArgLen = 128;

class ArgParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = nargparse::CreateParser("test_program", kMaxArgLen);
    }

    void TearDown() override {
        nargparse::FreeParser(parser_);
    }

    nargparse::ArgumentParser parser_;
};

#define parser parser_


bool IsEven(const int& value) {
    return value % 2 == 0;
}

bool IsPositive(const int& value) {
    return value > 0;
}

bool IsValidPort(const int& value) {
    return value > 0 && value <= 65535;
}


bool IsPositiveFloat(const float& value) {
    return value > 0.0f;
}

bool IsValidTemperature(const float& value) {
    return value >= -273.15f && value <= 1000.0f;
}

bool IsNormalizedFloat(const float& value) {
    return value >= 0.0f && value <= 1.0f;
}


bool IsNotEmpty(const char* const& value) {
    return std::strlen(value) != 0;
}

bool IsValidEmail(const char* const& value) {
    return (std::strchr(value, '@') != nullptr) && (std::strlen(value) > 3);
}

bool IsAlphaOnly(const char* const& value) {
    if (std::strlen(value) == 0) return false;

    for (const char* c = value; *c != '\0'; ++c) {
        if (!std::isalpha(*c)) return false;
    }
    return true;
}

TEST_F(ArgParserTest, EmptyParser) {
    const char* argv[] = {"program"};
    EXPECT_TRUE(nargparse::Parse(parser_, 1, argv));
}

TEST_F(ArgParserTest, FlagArguments) {
    bool flag1 = false;
    bool flag2 = false;

    nargparse::AddFlag(parser_, "-f", "--flag1", &flag1, "First flag", false);
    nargparse::AddFlag(parser_, "-g", "--flag2", &flag2, "Second flag", true);

    EXPECT_FALSE(flag1);
    EXPECT_TRUE(flag2);

    const char* argv[] = {"program", "-f", "--flag2"};
    EXPECT_TRUE(nargparse::Parse(parser_, 3, argv));
    EXPECT_TRUE(flag1);
    EXPECT_TRUE(flag2);
}

TEST_F(ArgParserTest, PositionalArguments) {
    int value = 0;

    nargparse::AddArgument(parser, &value, "Number");

    const char* argv[] = {"program", "42"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv));

    EXPECT_EQ(value, 42);
}

TEST_F(ArgParserTest, RepeatedArguments) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "Numbers", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program", "1", "2", "3", "4", "5"};
    EXPECT_TRUE(nargparse::Parse(parser, 6, argv));
    EXPECT_EQ(first_value, 1);

    int count = nargparse::GetRepeatedCount(parser, "Numbers");
    EXPECT_EQ(count, 5);
    for (int i = 0; i < count; ++i) {
        int value;
        EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", i, &value));
        EXPECT_EQ(value, i + 1);
    }
}

TEST_F(ArgParserTest, ValidationFunctionSuccess) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "Even numbers", nargparse::kNargsZeroOrMore, IsEven, "must be even");

    const char* argv[] = {"program", "2", "4", "6"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_EQ(first_value, 2);
    int count = nargparse::GetRepeatedCount(parser, "Even numbers");
    EXPECT_EQ(count, 3);
}

TEST_F(ArgParserTest, ValidationFunctionFailure) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "Even numbers", nargparse::kNargsZeroOrMore, IsEven, "must be even");

    const char* argv[] = {"program", "2", "3", "4"};
    EXPECT_FALSE(nargparse::Parse(parser, 4, argv));
}

TEST_F(ArgParserTest, HelpFlag) {
    nargparse::AddHelp(parser);

    const char* argv[] = {"program", "--help"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv));
    // Ождается что на такую команду будет выведен help
}


TEST_F(ArgParserTest, MixedArguments) {
    bool verbose = false;
    bool quiet = false;
    int first_number = 0;

    nargparse::AddFlag(parser, "-v", "--verbose", &verbose, "Verbose output");
    nargparse::AddFlag(parser, "-q", "--quiet", &quiet, "Quiet output");

    nargparse::AddArgument(parser, &first_number, "Numbers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "-v", "10", "20", "30"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));

    EXPECT_TRUE(verbose);
    EXPECT_FALSE(quiet);
    EXPECT_EQ(first_number, 10);

    int count = nargparse::GetRepeatedCount(parser, "Numbers");
    EXPECT_EQ(count, 3);

    int val;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 0, &val));
    EXPECT_EQ(val, 10);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 1, &val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 2, &val));
    EXPECT_EQ(val, 30);
}


TEST_F(ArgParserTest, InvalidInteger) {
    int value = 0;

    nargparse::AddArgument(parser, &value, "Number");

    const char* argv[] = {"program", "not_a_number"};
    EXPECT_FALSE(nargparse::Parse(parser, 2, argv));
}


TEST_F(ArgParserTest, EmptyArguments) {
    bool flag = false;
    nargparse::AddFlag(parser, "-f", "--flag", &flag, "A flag");

    const char* argv[] = {"program"};
    EXPECT_TRUE(nargparse::Parse(parser, 1, argv));
    EXPECT_FALSE(flag);
}


TEST_F(ArgParserTest, ShortFlag) {
    bool flag = false;
    nargparse::AddFlag(parser, "-f", "--flag", &flag, "A flag");

    const char* argv[] = {"program", "-f"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv));
    EXPECT_TRUE(flag);
}

TEST_F(ArgParserTest, LongFlag) {
    bool flag = false;
    nargparse::AddFlag(parser, "-f", "--flag", &flag, "A flag");

    const char* argv[] = {"program", "--flag"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv));
    EXPECT_TRUE(flag);
}


TEST_F(ArgParserTest, MultipleFlags) {
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;

    nargparse::AddFlag(parser, "-a", "--alpha", &flag1, "Alpha flag");
    nargparse::AddFlag(parser, "-b", "--beta", &flag2, "Beta flag");
    nargparse::AddFlag(parser, "-c", "--gamma", &flag3, "Gamma flag");

    const char* argv[] = {"program", "-a", "--beta", "-c"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_TRUE(flag1);
    EXPECT_TRUE(flag2);
    EXPECT_TRUE(flag3);
}


TEST_F(ArgParserTest, CustomValidationSuccess) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "Positive numbers", nargparse::kNargsZeroOrMore, IsPositive, "must be positive");

    const char* argv[] = {"program", "1", "2", "3"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_EQ(first_value, 1);
    int count = nargparse::GetRepeatedCount(parser, "Positive numbers");
    EXPECT_EQ(count, 3);
}

TEST_F(ArgParserTest, CustomValidationFailure) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "Positive numbers", nargparse::kNargsZeroOrMore, IsPositive, "must be positive");

    const char* argv[] = {"program", "1", "-2", "3"};
    EXPECT_FALSE(nargparse::Parse(parser, 4, argv));
}

TEST_F(ArgParserTest, EdgeCases) {
    const char* argv[] = {"program"};
    EXPECT_TRUE(nargparse::Parse(parser, 1, argv));

    bool flag = false;
    nargparse::AddFlag(parser, "-f", "--flag", &flag, "A flag");

    const char* argv2[] = {"program", "-f"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv2));
    EXPECT_TRUE(flag);
}

TEST_F(ArgParserTest, NamedArguments) {
    int count = 0;
    char name[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-n", "--count", &count, "Count value");
    nargparse::AddArgument(parser, "-s", "--name", &name, "Name value");

    const char* argv[] = {"program", "-n", "42", "--name", "test"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));
    EXPECT_EQ(count, 42);
    EXPECT_STREQ(name, "test");
}

TEST_F(ArgParserTest, RequiredNamedArgumentsProvided) {
    int port = 0;

    nargparse::AddArgument(parser, "-p", "--port", &port, "Port number", nargparse::kNargsRequired);

    const char* argv[] = {"program", "--port", "8080"};
    EXPECT_TRUE(nargparse::Parse(parser, 3, argv));
    EXPECT_EQ(port, 8080);
}

TEST_F(ArgParserTest, RequiredNamedArgumentsMissing) {
    int port = 0;

    nargparse::AddArgument(parser, "-p", "--port", &port, "Port number", nargparse::kNargsRequired);

    const char* argv[] = {"program"};
    EXPECT_FALSE(nargparse::Parse(parser, 1, argv));
}

TEST_F(ArgParserTest, MixedAllTypes) {
    bool verbose = false;
    int port = 0;
    char filename[kMaxArgLen] = {};

    nargparse::AddFlag(parser, "-v", "--verbose", &verbose, "Verbose output");
    nargparse::AddArgument(parser, "-p", "--port", &port, "Port number");
    nargparse::AddArgument(parser, &filename, "Input file");

    const char* argv[] = {"program", "-v", "input.txt", "-p", "3000"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));

    EXPECT_TRUE(verbose);
    EXPECT_EQ(port, 3000);
    EXPECT_STREQ(filename, "input.txt");
}

TEST_F(ArgParserTest, NamedArgumentValidationSuccess) {
    int port = 0;

    nargparse::AddArgument(parser, "-p", "--port", &port, "Port number", nargparse::kNargsOptional, IsValidPort);

    const char* argv[] = {"program", "-p", "8080"};
    EXPECT_TRUE(nargparse::Parse(parser, 3, argv));
    EXPECT_EQ(port, 8080);
}

TEST_F(ArgParserTest, NamedArgumentValidationFailure) {
    int port = 0;

    nargparse::AddArgument(parser, "-p", "--port", &port, "Port number", nargparse::kNargsOptional, IsValidPort);

    const char* argv[] = {"program", "-p", "99999"};
    EXPECT_FALSE(nargparse::Parse(parser, 3, argv));
}

TEST_F(ArgParserTest, MixedFlagsNamedAndPositional) {
    bool verbose = false;
    char name[kMaxArgLen] = {};
    int number = 0;

    nargparse::AddFlag(parser, "-v", "--verbose", &verbose, "Verbose");
    nargparse::AddArgument(parser, "-n", "--name", &name, "Name");
    nargparse::AddArgument(parser, &number, "number");

    const char* argv[] = {"program", "-v", "--name", "itmo", "44"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));

    EXPECT_TRUE(verbose);
    EXPECT_STREQ(name, "itmo");
    EXPECT_EQ(number, 44);
}

TEST_F(ArgParserTest, MultipleNamedArgumentCalls) {
    bool mult = false;
    int first_number = 0;

    nargparse::AddFlag(parser, "-m", "--mult", &mult, "Multiply");
    nargparse::AddArgument(parser,  "-n", "--number", &first_number, "Numbers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "--mult", "-n", "1", "-n", "2", "-n", "3", "-n", "4", "-n", "5"};
    EXPECT_TRUE(nargparse::Parse(parser, 12, argv));

    EXPECT_TRUE(mult);
    EXPECT_EQ(first_number, 1);

    int count = nargparse::GetRepeatedCount(parser, "Numbers");
    EXPECT_EQ(count, 5);
    for (int i = 0; i < count; ++i) {
        int val;
        EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", i, &val));
        EXPECT_EQ(val, i + 1);
    }
}

TEST_F(ArgParserTest, EqualsSignSyntax) {
    bool verbose = false;
    char name[kMaxArgLen] = {};
    int number = 0;

    nargparse::AddFlag(parser, "-v", "--verbose", &verbose, "Verbose");
    nargparse::AddArgument(parser, "-n", "--name", &name, "Name");
    nargparse::AddArgument(parser, &number, "number");

    const char* argv[] = {"program", "-v", "--name=itmo", "44"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_TRUE(verbose);
    EXPECT_STREQ(name, "itmo");
    EXPECT_EQ(number, 44);
}

TEST_F(ArgParserTest, DuplicateSingleArgument) {
    bool verbose = false;
    char name[kMaxArgLen] = {};
    int number = 0;

    nargparse::AddFlag(parser, "-v", "--verbose", &verbose, "Verbose");
    nargparse::AddArgument(parser, "-n", "--name", &name, "Name");
    nargparse::AddArgument(parser, &number, "number");

    const char* argv[] = {"program", "-v", "--name", "itmo", "44", "--name=itmo2"};
    EXPECT_FALSE(nargparse::Parse(parser, 6, argv));
}

TEST_F(ArgParserTest, FloatValidationSuccess) {
    float temperature = 0.0f;

    nargparse::AddArgument(parser, "-t", "--temp", &temperature, "Temperature", nargparse::kNargsOptional, IsValidTemperature);

    const char* argv[] = {"program", "-t", "25.5"};
    EXPECT_TRUE(nargparse::Parse(parser, 3, argv));
    EXPECT_NEAR(temperature, 25.5f, 0.01f);
}

TEST_F(ArgParserTest, FloatValidationFailure) {
    float temperature = 0.0f;

    nargparse::AddArgument(parser, "-t", "--temp", &temperature, "Temperature", nargparse::kNargsOptional, IsValidTemperature);

    const char* argv[] = {"program", "-t", "2000.0"};
    EXPECT_FALSE(nargparse::Parse(parser, 3, argv));
}

TEST_F(ArgParserTest, FloatVectorValidationSuccess) {
    float first_score = 0.0f;

    nargparse::AddArgument(parser,  "-s", "--score", &first_score, "Scores", nargparse::kNargsZeroOrMore, IsNormalizedFloat);

    const char* argv[] = {"program", "-s", "0.95", "-s", "0.87", "-s", "1.0"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));

    EXPECT_NEAR(first_score, 0.95f, 0.01f);
    int count = nargparse::GetRepeatedCount(parser, "Scores");
    EXPECT_EQ(count, 3);

    float val;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 0, &val));
    EXPECT_NEAR(val, 0.95f, 0.01f);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 1, &val));
    EXPECT_NEAR(val, 0.87f, 0.01f);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 2, &val));
    EXPECT_NEAR(val, 1.0f, 0.01f);
}

TEST_F(ArgParserTest, FloatVectorValidationFailure) {
    float first_score = 0.0f;

    nargparse::AddArgument(parser, "-s", "--score", &first_score, "Scores", nargparse::kNargsZeroOrMore, IsNormalizedFloat);

    const char* argv[] = {"program", "-s", "0.5", "-s", "1.5"};
    EXPECT_FALSE(nargparse::Parse(parser, 5, argv));
}

TEST_F(ArgParserTest, StringValidationSuccess) {
    char email[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-e", "--email", &email, "Email", nargparse::kNargsOptional,
                           IsValidEmail);

    const char* argv[] = {"program", "-e", "user@example.com"};
    EXPECT_TRUE(nargparse::Parse(parser, 3, argv));
    EXPECT_STREQ(email, "user@example.com");
}

TEST_F(ArgParserTest, StringValidationFailure) {
    char email[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-e", "--email", &email, "Email", nargparse::kNargsOptional,
                           IsValidEmail);

    const char* argv[] = {"program", "-e", "invalid"};
    EXPECT_FALSE(nargparse::Parse(parser, 3, argv));
}

TEST_F(ArgParserTest, StringGracefulOverflow) {
    char email[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-e", "--email", &email, "Email", nargparse::kNargsOptional);

    char too_big[kMaxArgLen + 1] = {};
    std::memset(too_big, 'a', kMaxArgLen);

    const char* argv[] = {"program", "-e", too_big};
    EXPECT_FALSE(nargparse::Parse(parser, 3, argv));
}

TEST_F(ArgParserTest, StringVectorValidationSuccess) {
    char first_name[kMaxArgLen] = {};

    nargparse::AddArgument(parser, &first_name, "names", nargparse::kNargsOneOrMore,
                           IsAlphaOnly);

    const char* argv[] = {"program", "Alice", "Bob", "Charlie"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_STREQ(first_name, "Alice");
    int count = nargparse::GetRepeatedCount(parser, "names");
    EXPECT_EQ(count, 3);

    const char* name;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 0, &name));
    EXPECT_STREQ(name, "Alice");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 1, &name));
    EXPECT_STREQ(name, "Bob");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 2, &name));
    EXPECT_STREQ(name, "Charlie");
}

TEST_F(ArgParserTest, StringVectorValidationFailure) {
    char first_name[kMaxArgLen] = {};

    nargparse::AddArgument(parser, &first_name, "names", nargparse::kNargsOneOrMore,
                           IsAlphaOnly);

    const char* argv[] = {"program", "Alice", "Bob123"};
    EXPECT_FALSE(nargparse::Parse(parser, 3, argv));
}

TEST_F(ArgParserTest, StringVectorLifetimes) {
    {
        char first_name[kMaxArgLen] = {};

        nargparse::AddArgument(parser, &first_name, "names", nargparse::kNargsOneOrMore);

        char a[] = "program";
        char b[] = "Alice";
        char c[] = "Bob";
        char d[] = "Charlie";
        const char* argv[] = {a, b, c, d};

        EXPECT_TRUE(nargparse::Parse(parser, 4, argv));
        EXPECT_STREQ(first_name, "Alice");
    }

    int count = nargparse::GetRepeatedCount(parser, "names");
    EXPECT_EQ(count, 3);

    const char* name;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 0, &name));
    EXPECT_STREQ(name, "Alice");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 1, &name));
    EXPECT_STREQ(name, "Bob");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "names", 2, &name));
    EXPECT_STREQ(name, "Charlie");
}

TEST_F(ArgParserTest, MixedTypesWithValidation) {
    int count = 0;
    float ratio = 0.0f;
    char name[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-c", "--count", &count, "Count", nargparse::kNargsOptional, IsPositive, "must be positive");
    nargparse::AddArgument(parser, "-r", "--ratio", &ratio, "Ratio", nargparse::kNargsOptional, IsNormalizedFloat, "must be between 0 and 1");
    nargparse::AddArgument(parser, "-n", "--name", &name, "Name", nargparse::kNargsOptional,
                           IsNotEmpty, "cannot be empty");

    const char* argv[] = {"program", "-c", "42", "-r", "0.75", "-n", "TestApp"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));

    EXPECT_EQ(count, 42);
    EXPECT_NEAR(ratio, 0.75f, 0.01f);
    EXPECT_STREQ(name, "TestApp");
}

TEST_F(ArgParserTest, PositionalFloatArguments) {
    float temperature = 0.0f;
    float first_reading = 0.0f;

    nargparse::AddArgument(parser, &temperature, "temperature");
    nargparse::AddArgument(parser, &first_reading, "readings", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "36.6", "98.6", "99.1", "97.8"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));

    EXPECT_NEAR(temperature, 36.6f, 0.01f);
    EXPECT_NEAR(first_reading, 98.6f, 0.01f);

    int count = nargparse::GetRepeatedCount(parser, "readings");
    EXPECT_EQ(count, 3);

    float val;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "readings", 0, &val));
    EXPECT_NEAR(val, 98.6f, 0.01f);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "readings", 1, &val));
    EXPECT_NEAR(val, 99.1f, 0.01f);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "readings", 2, &val));
    EXPECT_NEAR(val, 97.8f, 0.01f);
}

TEST_F(ArgParserTest, StructureBasedOptions) {
    struct ServerConfig {
        char host[kMaxArgLen];
        int port;
        bool verbose;
        bool debug;
        float timeout;
    };

    ServerConfig config = {};

    nargparse::AddArgument(parser, nullptr, "--host", &config.host, "Server host", nargparse::kNargsOptional,
                           IsNotEmpty, "cannot be empty");
    nargparse::AddArgument(parser, "-p", "--port", &config.port, "Server port", nargparse::kNargsOptional, IsValidPort, "must be 1-65535");
    nargparse::AddFlag(parser, "-v", "--verbose", &config.verbose, "Verbose output");
    nargparse::AddFlag(parser, "-d", "--debug", &config.debug, "Debug mode");
    nargparse::AddArgument(parser, "-t", "--timeout", &config.timeout, "Timeout in seconds", nargparse::kNargsOptional, IsPositiveFloat, "must be positive");

    const char* argv[] = {"program", "--host", "localhost", "-p", "8080", "-v", "-t", "30.5"};
    EXPECT_TRUE(nargparse::Parse(parser, 8, argv));

    EXPECT_STREQ(config.host, "localhost");
    EXPECT_EQ(config.port, 8080);
    EXPECT_TRUE(config.verbose);
    EXPECT_FALSE(config.debug);
    EXPECT_NEAR(config.timeout, 30.5f, 0.01f);
}

TEST_F(ArgParserTest, NestedStructureOptions) {
    struct DatabaseConfig {
        char host[kMaxArgLen];
        int port;
        char name[kMaxArgLen];
    };

    struct LogConfig {
        bool verbose;
        char logfile[kMaxArgLen];
        int level;
    };

    struct ApplicationConfig {
        DatabaseConfig database;
        LogConfig logging;
        char app_name[kMaxArgLen];
        char first_tag[kMaxArgLen];
    };

    ApplicationConfig app_config = {};

    nargparse::AddArgument(parser, nullptr, "--db-host", &app_config.database.host, "Database host");
    nargparse::AddArgument(parser, nullptr, "--db-port", &app_config.database.port, "Database port", nargparse::kNargsOptional, IsValidPort, "must be 1-65535");
    nargparse::AddArgument(parser, nullptr, "--db-name", &app_config.database.name, "Database name");

    nargparse::AddFlag(parser, "-v", "--verbose", &app_config.logging.verbose, "Verbose logging");
    nargparse::AddArgument(parser, nullptr, "--logfile", &app_config.logging.logfile, "Log file path");
    nargparse::AddArgument(parser, nullptr, "--log-level", &app_config.logging.level, "Log level", nargparse::kNargsOptional, IsPositive, "must be positive");

    nargparse::AddArgument(parser, "-n", "--name", &app_config.app_name, "Application name");
    nargparse::AddArgument(parser, "-t", "--tag", &app_config.first_tag, "Tags", nargparse::kNargsZeroOrMore);

    const char* argv[] = {
        "program",
        "--db-host", "postgres.example.com",
        "--db-port", "5432",
        "--db-name", "myapp",
        "-v",
        "--logfile", "/var/log/app.log",
        "--log-level", "3",
        "-n", "MyApplication",
        "-t", "production", "-t", "backend", "-t", "api"
    };

    EXPECT_TRUE(nargparse::Parse(parser, 20, argv));

    EXPECT_STREQ(app_config.database.host, "postgres.example.com");
    EXPECT_EQ(app_config.database.port, 5432);
    EXPECT_STREQ(app_config.database.name, "myapp");

    EXPECT_TRUE(app_config.logging.verbose);
    EXPECT_STREQ(app_config.logging.logfile, "/var/log/app.log");
    EXPECT_EQ(app_config.logging.level, 3);

    EXPECT_STREQ(app_config.app_name, "MyApplication");
    EXPECT_STREQ(app_config.first_tag, "production");

    int tag_count = nargparse::GetRepeatedCount(parser, "Tags");
    ASSERT_EQ(tag_count, 3);

    const char* tag;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 0, &tag));
    EXPECT_STREQ(tag, "production");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 1, &tag));
    EXPECT_STREQ(tag, "backend");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 2, &tag));
    EXPECT_STREQ(tag, "api");
}


TEST_F(ArgParserTest, ComplexStructureWithMixedTypes) {
    struct NetworkSettings {
        char interface[kMaxArgLen];
        int mtu;
        float bandwidth;
    };

    struct SecuritySettings {
        bool enable_ssl;
        bool enable_auth;
        char first_allowed_host[kMaxArgLen];
    };

    struct ServiceConfig {
        NetworkSettings network;
        SecuritySettings security;
        int first_worker_port;
        char service_name[kMaxArgLen];
    };

    ServiceConfig service = {};

    nargparse::AddArgument(parser, nullptr, "--interface", &service.network.interface, "Network interface");
    nargparse::AddArgument(parser, nullptr, "--mtu", &service.network.mtu, "MTU size", nargparse::kNargsOptional, IsPositive, "must be positive");
    nargparse::AddArgument(parser, nullptr, "--bandwidth", &service.network.bandwidth, "Bandwidth limit", nargparse::kNargsOptional, IsPositiveFloat, "must be positive");

    nargparse::AddFlag(parser, nullptr, "--ssl", &service.security.enable_ssl, "Enable SSL");
    nargparse::AddFlag(parser, nullptr, "--auth", &service.security.enable_auth, "Enable authentication");
    nargparse::AddArgument(parser, nullptr, "--allow", &service.security.first_allowed_host, "Allowed hosts", nargparse::kNargsZeroOrMore);

    nargparse::AddArgument(parser,  "-w", "--worker-port", &service.first_worker_port, "Worker ports", nargparse::kNargsZeroOrMore, IsValidPort, "must be 1-65535");
    nargparse::AddArgument(parser, "-s", "--service", &service.service_name, "Service name", nargparse::kNargsOptional,
                           IsAlphaOnly, "must contain only letters");

    const char* argv[] = {
        "program",
        "--interface", "eth0",
        "--mtu", "1500",
        "--bandwidth", "100.5",
        "--ssl",
        "--auth",
        "--allow", "192.168.1.1", "--allow", "10.0.0.1",
        "-w", "8080", "-w", "8081", "-w", "8082",
        "-s", "WebService"
    };

    EXPECT_TRUE(nargparse::Parse(parser, 21, argv));

    EXPECT_STREQ(service.network.interface, "eth0");
    EXPECT_EQ(service.network.mtu, 1500);
    EXPECT_NEAR(service.network.bandwidth, 100.5f, 0.01f);

    EXPECT_TRUE(service.security.enable_ssl);
    EXPECT_TRUE(service.security.enable_auth);
    EXPECT_STREQ(service.security.first_allowed_host, "192.168.1.1");

    int allow_count = nargparse::GetRepeatedCount(parser, "Allowed hosts");
    EXPECT_EQ(allow_count, 2);
    const char* host;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Allowed hosts", 0, &host));
    EXPECT_STREQ(host, "192.168.1.1");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Allowed hosts", 1, &host));
    EXPECT_STREQ(host, "10.0.0.1");

    EXPECT_EQ(service.first_worker_port, 8080);
    int port_count = nargparse::GetRepeatedCount(parser, "Worker ports");
    EXPECT_EQ(port_count, 3);
    int port;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Worker ports", 0, &port));
    EXPECT_EQ(port, 8080);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Worker ports", 1, &port));
    EXPECT_EQ(port, 8081);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Worker ports", 2, &port));
    EXPECT_EQ(port, 8082);

    EXPECT_STREQ(service.service_name, "WebService");
}


TEST_F(ArgParserTest, NargsOneOrMoreWithNoValues) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "numbers", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program"};
    EXPECT_FALSE(nargparse::Parse(parser, 1, argv));
}

TEST_F(ArgParserTest, NargsOneOrMoreWithSingleValue) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "numbers", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program", "42"};
    EXPECT_TRUE(nargparse::Parse(parser, 2, argv));
    int count = nargparse::GetRepeatedCount(parser, "numbers");
    EXPECT_EQ(count, 1);
    EXPECT_EQ(first_value, 42);
}

TEST_F(ArgParserTest, NargsOneOrMoreWithMultipleValues) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "numbers", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program", "1", "2", "3", "4", "5"};
    EXPECT_TRUE(nargparse::Parse(parser, 6, argv));
    int count = nargparse::GetRepeatedCount(parser, "numbers");
    EXPECT_EQ(count, 5);
    EXPECT_EQ(first_value, 1);
    int val;
    nargparse::GetRepeated(parser, "numbers", 4, &val);
    EXPECT_EQ(val, 5);
}

TEST_F(ArgParserTest, NamedNargsOneOrMoreWithNoValues) {
    char first_tag[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-t", "--tag", &first_tag, "Tags", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program"};
    EXPECT_FALSE(nargparse::Parse(parser, 1, argv));
}

TEST_F(ArgParserTest, NamedNargsOneOrMoreWithValues) {
    char first_tag[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-t", "--tag", &first_tag, "Tags", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program", "-t", "alpha", "-t", "beta", "-t", "gamma"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));
    EXPECT_STREQ(first_tag, "alpha");
    int count = nargparse::GetRepeatedCount(parser, "Tags");
    EXPECT_EQ(count, 3);
    const char* tag;
    nargparse::GetRepeated(parser, "Tags", 0, &tag);
    EXPECT_STREQ(tag, "alpha");
    nargparse::GetRepeated(parser, "Tags", 1, &tag);
    EXPECT_STREQ(tag, "beta");
    nargparse::GetRepeated(parser, "Tags", 2, &tag);
    EXPECT_STREQ(tag, "gamma");
}

TEST_F(ArgParserTest, FunnyArgumentValue) {
    char pos[kMaxArgLen] = {};
    char arg[kMaxArgLen] = {};
    char fun[kMaxArgLen] = {};

    nargparse::AddArgument(parser, &pos, "pos", nargparse::kNargsZeroOrMore);
    nargparse::AddArgument(parser, nullptr, "--arg", &arg, "Serious business only.", nargparse::kNargsOptional);
    nargparse::AddArgument(parser, nullptr, "--funny", &fun, "Nothing funny here.", nargparse::kNargsOptional);

    const char* argv[] = {"program", "--arg", "--funny", "positional"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_STREQ(pos, "positional");
    EXPECT_STREQ(arg, "--funny");
    EXPECT_STREQ(fun, "");
}

TEST_F(ArgParserTest, CStyleIterationNamedInt) {
    int first_number = 0;

    nargparse::AddArgument(parser,  "-n", "--number", &first_number, "Numbers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "-n", "10", "-n", "20", "-n", "30"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));

    EXPECT_EQ(first_number, 10);
    int count = nargparse::GetRepeatedCount(parser, "Numbers");
    EXPECT_EQ(count, 3);

    int value;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 0, &value));
    EXPECT_EQ(value, 10);

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 1, &value));
    EXPECT_EQ(value, 20);

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Numbers", 2, &value));
    EXPECT_EQ(value, 30);

    EXPECT_FALSE(nargparse::GetRepeated(parser, "Numbers", 3, &value));
}

TEST_F(ArgParserTest, CStyleIterationNamedFloat) {
    float first_score = 0.0f;

    nargparse::AddArgument(parser, "-s", "--score", &first_score, "Scores", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "-s", "1.5", "-s", "2.7", "-s", "3.9"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));

    EXPECT_NEAR(first_score, 1.5f, 0.01f);
    int count = nargparse::GetRepeatedCount(parser, "Scores");
    EXPECT_EQ(count, 3);

    float value;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 0, &value));
    EXPECT_NEAR(value, 1.5f, 0.01f);

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 1, &value));
    EXPECT_NEAR(value, 2.7f, 0.01f);

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Scores", 2, &value));
    EXPECT_NEAR(value, 3.9f, 0.01f);
}

TEST_F(ArgParserTest, CStyleIterationNamedString) {
    char first_tag[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-t", "--tag", &first_tag, "Tags", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "-t", "alpha", "-t", "beta", "-t", "gamma"};
    EXPECT_TRUE(nargparse::Parse(parser, 7, argv));

    EXPECT_STREQ(first_tag, "alpha");
    int count = nargparse::GetRepeatedCount(parser, "Tags");
    EXPECT_EQ(count, 3);

    const char* value;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 0, &value));
    EXPECT_STREQ(value, "alpha");

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 1, &value));
    EXPECT_STREQ(value, "beta");

    EXPECT_TRUE(nargparse::GetRepeated(parser, "Tags", 2, &value));
    EXPECT_STREQ(value, "gamma");
}

TEST_F(ArgParserTest, CStyleIterationPositionalInt) {
    int first_value = 0;

    nargparse::AddArgument(parser, &first_value, "numbers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "100", "200", "300", "400"};
    EXPECT_TRUE(nargparse::Parse(parser, 5, argv));

    EXPECT_EQ(first_value, 100);
    int count = nargparse::GetRepeatedCount(parser, "numbers");
    EXPECT_EQ(count, 4);

    int value;
    for (int i = 0; i < count; ++i) {
        EXPECT_TRUE(nargparse::GetRepeated(parser, "numbers", i, &value));
        EXPECT_EQ(value, (i + 1) * 100);
    }
}

TEST_F(ArgParserTest, CStyleIterationPositionalString) {
    char first_file[kMaxArgLen] = {};

    nargparse::AddArgument(parser, &first_file, "files", nargparse::kNargsOneOrMore);

    const char* argv[] = {"program", "file1.txt", "file2.txt", "file3.txt"};
    EXPECT_TRUE(nargparse::Parse(parser, 4, argv));

    EXPECT_STREQ(first_file, "file1.txt");
    int count = nargparse::GetRepeatedCount(parser, "files");
    EXPECT_EQ(count, 3);

    const char* filename;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "files", 0, &filename));
    EXPECT_STREQ(filename, "file1.txt");

    EXPECT_TRUE(nargparse::GetRepeated(parser, "files", 1, &filename));
    EXPECT_STREQ(filename, "file2.txt");

    EXPECT_TRUE(nargparse::GetRepeated(parser, "files", 2, &filename));
    EXPECT_STREQ(filename, "file3.txt");
}

TEST_F(ArgParserTest, CStyleIterationEmpty) {
    int first_number = 0;

    nargparse::AddArgument(parser, "-n", "--number", &first_number, "Numbers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program"};
    EXPECT_TRUE(nargparse::Parse(parser, 1, argv));

    int count = nargparse::GetRepeatedCount(parser, "Numbers");
    EXPECT_EQ(count, 0);

    int value;
    EXPECT_FALSE(nargparse::GetRepeated(parser, "Numbers", 0, &value));
}

TEST_F(ArgParserTest, CStyleIterationMixed) {
    int first_port = 0;
    char first_server[kMaxArgLen] = {};

    nargparse::AddArgument(parser, "-p", "--port", &first_port, "Ports", nargparse::kNargsZeroOrMore);
    nargparse::AddArgument(parser, "-s", "--server", &first_server, "Servers", nargparse::kNargsZeroOrMore);

    const char* argv[] = {"program", "-p", "8080", "-s", "localhost", "-p", "9090", "-s", "example.com"};
    EXPECT_TRUE(nargparse::Parse(parser, 9, argv));

    EXPECT_EQ(first_port, 8080);
    EXPECT_STREQ(first_server, "localhost");
    EXPECT_EQ(nargparse::GetRepeatedCount(parser, "Ports"), 2);
    EXPECT_EQ(nargparse::GetRepeatedCount(parser, "Servers"), 2);

    int port;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Ports", 0, &port));
    EXPECT_EQ(port, 8080);
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Ports", 1, &port));
    EXPECT_EQ(port, 9090);

    const char* server;
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Servers", 0, &server));
    EXPECT_STREQ(server, "localhost");
    EXPECT_TRUE(nargparse::GetRepeated(parser, "Servers", 1, &server));
    EXPECT_STREQ(server, "example.com");
}
