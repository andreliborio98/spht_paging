// A stupid and simple unit test framework for C++ code.
// Evan Jones <ej@evanjones.ca>

#include "stupidunit.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "assert.h"

using std::string;

// Contains and runs a collection of tests.
void TestSuite::registerTest(Test* (*test_factory)()) {
    ASSERT(test_factory != NULL);
    test_factories_.push_back(test_factory);
}

// JSON requires the following characters to be escaped in strings:
// quotation mark, revrse solidus, and U+0000 through U+001F.
// http://www.ietf.org/rfc/rfc4627.txt
static void jsonEscape(string* s) {
    static constexpr char ESCAPES[] = { '"', '\\', '\b', '\f', '\n', '\r', '\t' };
    static constexpr char REPLACEMENTS[] = { '"', '\\', 'b', 'f', 'n', 'r', 't' };
    ASSERT(sizeof(ESCAPES) == sizeof(REPLACEMENTS));

    for (size_t i = 0; i < s->size(); ++i) {
        char c = (*s)[i];
        if ((0 <= c && c <= 0x1f) || c == '"' || c =='\\') {
            // The character must be replaced with something: look in escapes
            bool replaced = false;
            for (int j = 0; j < sizeof(ESCAPES); ++j) {
                if (ESCAPES[j] == c) {
                    replaced = true;
                    (*s)[i] = '\\';
                    i += 1;
                    s->insert(i, 1, REPLACEMENTS[j]);
                    break;
                }
            }
            if (!replaced) {
                // Use a hex escape sequence
                char hex[7];
                int bytes = snprintf(hex, sizeof(hex), "\\u%04x", (int) c);
                ASSERT(bytes == sizeof(hex)-1);
                s->replace(i, 1, hex, sizeof(hex)-1);
                // normally move forward 1 character, so move past the entire escape
                i += sizeof(hex) - 2;
            }
        }
    }
}

int TestSuite::runAll() {
    // Change to the root directory to avoid tests that depend on files, etc.
    int status = chdir("/");
    ASSERT(status == 0);

    // Look to see if we should produce machine readable output
    const char* machine_readable = getenv(stupidunit::OUT_FILE_ENVIRONMENT_VARIABLE);
    int json_output = -1;
    if (machine_readable != NULL && machine_readable[0] != '\0') {
        json_output = open(machine_readable, O_WRONLY | O_CREAT | O_EXCL, 0666);
        if (json_output == -1 && errno == EEXIST) {
            fprintf(stderr,
                    "ERROR: %s file (%s) already exists: remove this file to run the tests",
                    stupidunit::OUT_FILE_ENVIRONMENT_VARIABLE, machine_readable);
            abort();
        }
        ASSERT(json_output >= 0);

        ssize_t bytes = write(json_output, "[", 1);
        ASSERT(bytes == 1);
    }

    int failed_tests = 0;
    const char* last_suite = NULL;
    for (size_t i = 0; i < test_factories_.size(); ++i) {
        // Create the test
        Test* test = test_factories_[i]();
        ASSERT(test != NULL);

        // Print the suite name if it is new
        if (last_suite == NULL || strcmp(test->suiteName(), last_suite) != 0) {
            if (last_suite != NULL) printf("\n");
            last_suite = test->suiteName();
            printf("%s:\n", last_suite);
        }

        // Print the test name
        printf("\t%s: ", test->testName());
        fflush(stdout);

        // run the test and check the result
        test->run();
        if (test->testSuccess()) {
            printf("PASSED.\n");
        } else {
            printf("FAILED.\n");
            test->printErrors();
            printf("\n");
            failed_tests++;
        }

        if (json_output != -1) {
            string json = "{\"class_name\": \"";
            json += test->suiteName();
            json += "\", \"name\": \"";
            json += test->testName();
            json += "\"";
            if (!test->testSuccess()) {
                json += ", \"failure\": \"";
                for (int j = 0; j < test->stupidunitNumErrors(); ++j) {
                    string message = test->stupidunitError(j);
                    jsonEscape(&message);
                    json += message;
                    json += "\\n";
                }
                json += "\"";
            }
            json += "}";

            if (i != test_factories_.size() - 1) {
                json += ",\n";
            } else {
                json += "]\n";
            }

            ssize_t bytes = write(json_output, json.data(), json.size());
            ASSERT(bytes == json.size());
        }

        // Clean up the test
        delete test;
    }

    if (json_output != -1) {
        int error = close(json_output);
        ASSERT(error == 0);
    }

    if (failed_tests == 0) {
        printf("PASSED\n");
    } else {
        printf("%d FAILED\n", failed_tests);
    }
    return failed_tests;
}

TestSuite* TestSuite::globalInstance() {
    // Avoids static initialization order problems, although it could have destructor order
    // problems if the TestSuite destructor did anything. See:
    // http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.12
    static TestSuite global_suite;
    return &global_suite;
}

void Test::fail(const char* file, int line, const char* message) {
    std::ostringstream output;
    output << file << ":" << line << ": Test failed: " << message;
    errors_.push_back(output.str());
}

void Test::printErrors() const {
    for (size_t i = 0; i < errors_.size(); ++i) {
        printf("%s\n", errors_[i].c_str());
    }
}

const string& Test::stupidunitError(int i) const {
    ASSERT(0 <= i && i < errors_.size());
    return errors_[i];
}

namespace stupidunit {

// P_tmpdir comes from stdio.h (see tempnam). TODO: Look at environment variables?
ChTempDir::ChTempDir() : name_(P_tmpdir) {
    if (name_[name_.size()-1] != '/') {
        name_.push_back('/');
    }
    name_ += "test_XXXXXX";
    // Abuse the string to modify it in place
#ifndef __sun
    char* result = mkdtemp(&*name_.begin());
#else
    // Solaris doesn't seem to have mkdtemp?
    char* result = NULL;
    char* temp = &*name_.begin();
    if (mktemp(temp) && mkdir(temp, 0700) == 0) {
        result = temp;
    }
#endif
    // TODO: Abort/throw on error
    ASSERT(result != NULL);
    ASSERT(result == name_.c_str());

    int status = chdir(name_.c_str());
    ASSERT(status == 0);
}

// Recursively deletes the file named path. If path is a file, it will be
// removed. If it is a directory, everything in it will also be deleted.
// Returns 0 on success, -1 on error, and sets errno appropriately.
static int rmtree(const char* path) {
    // TODO: Handle errors in a more intelligent fashion?
    DIR* dir = opendir(path);
    if (dir == NULL) {
        if (errno == ENOTDIR) {
            // Not a directory: unlink it instead
            return unlink(path);
        }

        return -1;
    }
    ASSERT(dir != NULL);

    for (struct dirent* entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
        // Skip special directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Recursively delete the directory entry. This handles regular files
        // and directories.
        string fullpath(path);
        fullpath += "/";
        fullpath += entry->d_name;
        rmtree(fullpath.c_str());
    }

    int error = closedir(dir);
    ASSERT(error == 0);

    return rmdir(path);
}

ChTempDir::~ChTempDir() {
    // TODO: chdir back to the original directory?
    int status = chdir("/");
    ASSERT(status == 0);

    // Recursively delete everything in the temporary directory.
    status = rmtree(name_.c_str());
    ASSERT(status == 0);
}

// TODO: Capture and match client output
ExpectDeathStatus expectDeath() {
    // Create a pipe for the child's output
    int pipe_descriptors[2];
    int error = pipe(pipe_descriptors);
    ASSERT(error == 0);

    
    pid_t expect_death_pid = fork();
    if (expect_death_pid == 0) {
        // Capture the child's output: replace stdout and stderr with the pipe
        error = dup2(pipe_descriptors[1], 1);
        ASSERT(error == 1);
        error = dup2(pipe_descriptors[1], 2);
        ASSERT(error == 2);

        // This is the child: tell the macro to run the block
        return EXECUTE_BLOCK;
    }

    // Parent: close the write end of the pipe
    error = close(pipe_descriptors[1]);
    ASSERT(error == 0);

    // Read the child's output
    string output;
    char buffer[4096];
    ssize_t bytes = 0;
    while ((bytes = read(pipe_descriptors[0], buffer, sizeof(buffer))) > 0
            || (bytes == -1 && errno == EINTR)) {
        if (bytes == -1) {
            // On Mac OS X under gdb, read returns EINTR when the child dies: ignore it
            assert(errno == EINTR);
        } else {
            output.append(buffer, bytes);
        }
    }
    ASSERT(bytes == 0);
    error = close(pipe_descriptors[0]);
    ASSERT(error == 0);

    // Collect the child's output status
    int expect_death_status = -1;
    waitpid(expect_death_pid, &expect_death_status, 0);
    if (WIFEXITED(expect_death_status)) {
        // The block exited: it was supposed to die!
        return FAILED;
    }

    // The client failed in some way: great
    return SUCCESS;
}

const char OUT_FILE_ENVIRONMENT_VARIABLE[] = "STUPIDUNIT_OUTPUT";

}  // namespace stupidunit
