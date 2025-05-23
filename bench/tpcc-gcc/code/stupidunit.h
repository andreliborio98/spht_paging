// A stupid and simple unit test framework for C++ code.
// Evan Jones <ej@evanjones.ca>

#ifndef STUPIDUNIT_H__
#define STUPIDUNIT_H__

#include <cstdlib>
#include <string>
#include <vector>

class Test;

// Contains and runs a collection of tests.
class TestSuite {
public:
    void registerTest(Test* (*test_factory)());

    // Returns the number of failed tests.
    int runAll();

    // Returns a properly initialized static global TestSuite. This is the "standard" test suite
    // used by the TEST and TEST_F macros.
    static TestSuite* globalInstance();

private:
    std::vector<Test* (*)()> test_factories_;
};

// Base class for a single test. Each test creates a subclass of this that
// implements run(). Users create subclasses via the TEST_F helper macro.
class Test {
public:
    Test() {}
    virtual ~Test() {}

    // Run the actual test.
    virtual void run() = 0;

    virtual const char* suiteName() const = 0;
    virtual const char* testName() const = 0;
    bool testSuccess() const { return errors_.empty(); }

    // Fail the test with error.
    void fail(const char* file, int line, const char* message);

    // Output the errors for this test to standard output.
    void printErrors() const;

    int stupidunitNumErrors() const { return static_cast<int>(errors_.size()); }
    const std::string& stupidunitError(int i) const;

private:
    // Contains error messages if the test failed.
    std::vector<std::string> errors_;
};

// A class used to statically register test instances with the global test suite.
template <typename T>
class RegisterTest {
public:
    RegisterTest(TestSuite* suite) {
        suite->registerTest(&RegisterTest<T>::create);
    }

    static Test* create() {
        return new T();
    }
};

// Creates a test subclass.
#define MAGIC_TEST_MACRO(parent_class, suite_name, test_name) \
    class suite_name ## _ ## test_name : public parent_class { \
    public: \
        virtual ~suite_name ## _ ## test_name() {} \
        virtual void run(); \
        virtual const char* suiteName() const { return suite_name_; } \
        virtual const char* testName() const { return test_name_; } \
\
    private: \
        static constexpr char suite_name_[] = #suite_name; \
        static constexpr char test_name_[] = #test_name; \
    }; \
    /*const char suite_name ## _ ## test_name::suite_name_[] =  #suite_name;*/ \
    /*const char suite_name ## _ ## test_name::test_name_[] =  #test_name;*/ \
    static RegisterTest<suite_name ## _ ## test_name> suite_name ## _ ## test_name ## _register(TestSuite::globalInstance()); \
    void suite_name ## _ ## test_name::run()

// A magic macro to make a test part of a user-defined test subclass.
#define TEST_F(harness_name, test_name) MAGIC_TEST_MACRO(harness_name, harness_name, test_name)

// A magic macro to make a test subclass for a block of code.
#define TEST(suite_name, test_name) MAGIC_TEST_MACRO(Test, suite_name, test_name)

// Abuse macros to easily define all the EXPECT and ASSERT variants
#define STUPIDUNIT_MAKE_EXPECT_MACRO(operation, one, two) do { \
    if (!((one) operation (two))) fail(__FILE__, __LINE__, #one " " #operation " " #two); \
} while (0)

#define EXPECT_EQ(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(==, one, two)
#define EXPECT_NE(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(!=, one, two)
#define EXPECT_LT(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(<, one, two)
#define EXPECT_LE(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(<=, one, two)
#define EXPECT_GT(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(>, one, two)
#define EXPECT_GE(one, two) STUPIDUNIT_MAKE_EXPECT_MACRO(>=, one, two)

#define EXPECT_TRUE(value) do { \
    if (!(value)) fail(__FILE__, __LINE__, "Expected true; " #value " is false"); \
} while (0)
#define EXPECT_FALSE(value) do { \
    if ((value)) fail(__FILE__, __LINE__, "Expected false; " #value " is true"); \
} while (0)

// The only difference between EXPECT and ASSERT is that ASSERT returns from
// the test method if the test fails
#define STUPIDUNIT_MAKE_ASSERT_MACRO(operation, one, two) do { \
    if (!((one) operation (two))) { fail(__FILE__, __LINE__, #one " " #operation " " #two); return; } \
} while (0)

#define ASSERT_EQ(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(==, one, two)
#define ASSERT_NE(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(!=, one, two)
#define ASSERT_LT(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(<, one, two)
#define ASSERT_LE(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(<=, one, two)
#define ASSERT_GT(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(>, one, two)
#define ASSERT_GE(one, two) STUPIDUNIT_MAKE_ASSERT_MACRO(>=, one, two)

#define ASSERT_TRUE(value) do { \
    if (!(value)) { fail(__FILE__, __LINE__, "Expected true; " #value " is false"); return; } \
} while (0)
#define ASSERT_FALSE(value) do { \
    if ((value)) { fail(__FILE__, __LINE__, "Expected false; " #value " is true"); return; } \
} while (0)


namespace stupidunit {

enum ExpectDeathStatus {
    // The caller is the child: run the block and exit.
    EXECUTE_BLOCK,
    SUCCESS,
    FAILED
};

// Implements EXPECT_DEATH.
ExpectDeathStatus expectDeath();

// Helper that creates a temporary directory then changes into it. The
// directory will be automatically removed in the destructor.
class ChTempDir {
public:
    ChTempDir();
    ~ChTempDir();

    const std::string& name() const { return name_; }

private:
    // The name of the temporary directory.
    std::string name_;
};

extern const char OUT_FILE_ENVIRONMENT_VARIABLE[];

}  // namespace stupidunit

#define EXPECT_DEATH(block) do { \
    stupidunit::ExpectDeathStatus status = stupidunit::expectDeath(); \
    if (status == stupidunit::EXECUTE_BLOCK) { \
        block; \
        exit(0); \
    } else if (status == stupidunit::FAILED) { \
        fail(__FILE__, __LINE__, "EXPECT_DEATH(" #block "): did not die"); \
    } \
} while (0)

#endif  // STUPIDUNIT_H__
