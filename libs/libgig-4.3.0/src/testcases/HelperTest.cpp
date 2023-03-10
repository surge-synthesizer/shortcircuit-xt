#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <map>

// before including helper.h, pretend to be Windows to enable the drive (c:\) handling routines ...
#define _WIN32
// ... but at the same time force forward slash as our path separator for our tests
#define NATIVE_PATH_SEPARATOR '/'

#include "../helper.h"

int main() {
    int iErrors = 0;

    // test cases for function strip2ndFromEndOf1st()
    {
        // (a) input string, (b) expected result string
        std::pair<std::string, std::string> examples[] = {
            { "", "" },
            { "/", "" },
            { "//", "" },
            { "///", "" },
            { "/foo/bla", "/foo/bla" },
            { "/foo/bla/", "/foo/bla" },
            { "/foo/bla//", "/foo/bla" },
            { "/foo/bla///", "/foo/bla" },
            { "/foo", "/foo" },
            { "/foo/", "/foo" },
            { "/foo//", "/foo" },
            { "/foo///", "/foo" },
            { "/foo/asdf.gig", "/foo/asdf.gig" },
            { "foo", "foo" },
            { "asdf.gig", "asdf.gig" },
            { "C:/", "C:" },
            { "C://", "C:" },
            { "C:", "C:" },
            { "C:/asdf", "C:/asdf" },
            { "C:/asdf/", "C:/asdf"},
            { "C:/asdf//", "C:/asdf"},
            { "C:/asdf///", "C:/asdf"},
        };
        for (auto ex : examples) {
            const std::string in  = ex.first;
            const std::string out = strip2ndFromEndOf1st(in, '/');
            const std::string expected = ex.second;
            if (out != expected) iErrors++;
            printf("strip2ndFromEndOf1st('%s', '/')  =>  '%s' [%s]\n",
                   in.c_str(), out.c_str(),
                   ((out == expected) ? "ok" : "WRONG")
            );
        }
    }

    // test cases for function parentPath()
    {
        // (a) input string, (b) expected result string
        std::pair<std::string, std::string> examples[] = {
            { "", "" },
            { "/", "/" },
            { "//", "/" },
            { "///", "/" },
            { "/foo/bla", "/foo" },
            { "/foo/bla/", "/foo" },
            { "/foo/bla//", "/foo" },
            { "/foo/bla///", "/foo" },
            { "/foo", "/" },
            { "/foo/", "/" },
            { "/foo//", "/" },
            { "/foo///", "/" },
            { "/foo/asdf.gig", "/foo" },
            { "foo", "" },
            { "asdf.gig", "" },
            { "C:", "C:" },
            { "C:/", "C:" },
            { "C://", "C:" },
            { "C:///", "C:" },
            { "C:/asdf", "C:" },
            { "C:/asdf/", "C:"},
            { "C:/asdf//", "C:"},
            { "C:/asdf///", "C:"},
            { "C:/foo.gig", "C:"},
        };
        for (auto ex : examples) {
            const std::string in  = ex.first;
            const std::string out = parentPath(in);
            const std::string expected = ex.second;
            if (out != expected) iErrors++;
            printf("parentPath('%s') => '%s' [%s]\n",
                   in.c_str(), out.c_str(),
                   ((out == expected) ? "ok" : "WRONG")
            );
        }
    }

    // test cases for function lastPathComponent()
    {
        // (a) input string, (b) expected result string
        std::pair<std::string, std::string> examples[] = {
            { "", "" },
            { "/", "" },
            { "//", "" },
            { "///", "" },
            { "/foo/bla", "bla" },
            { "/foo/bla/", "" },
            { "/foo/bla//", "" },
            { "/foo/bla///", "" },
            { "/foo", "foo" },
            { "/foo/", "" },
            { "/foo//", "" },
            { "/foo///", "" },
            { "/foo/asdf.gig", "asdf.gig" },
            { "foo", "foo" },
            { "asdf.gig", "asdf.gig" },
            { "C:", "" },
            { "C:/", "" },
            { "C://", "" },
            { "C:///", "" },
            { "C:/asdf", "asdf" },
            { "C:/asdf/", ""},
            { "C:/asdf//", ""},
            { "C:/asdf///", ""},
            { "C:/foo.gig", "foo.gig"},
        };
        for (auto ex : examples) {
            const std::string in  = ex.first;
            const std::string out = lastPathComponent(in);
            const std::string expected = ex.second;
            if (out != expected) iErrors++;
            printf("lastPathComponent('%s') => '%s' [%s]\n",
                   in.c_str(), out.c_str(),
                   ((out == expected) ? "ok" : "WRONG")
            );
        }
    }

    // test cases for function pathWithoutExtension()
    {
        // (a) input string, (b) expected result string
        std::pair<std::string, std::string> examples[] = {
            { "", "" },
            { "/foo/bla", "/foo/bla" },
            { "/foo", "/foo" },
            { "/foo/asdf.gig", "/foo/asdf" },
            { "/foo/asdf.gigx", "/foo/asdf" },
            { "foo", "foo" },
            { "asdf.gig", "asdf" },
            { "asdf.blabla", "asdf" },
            { "C:/foo.gig", "C:/foo"},
            { "C:/foo.blabla", "C:/foo"},
        };
        for (auto ex : examples) {
            const std::string in  = ex.first;
            const std::string out = pathWithoutExtension(in);
            const std::string expected = ex.second;
            if (out != expected) iErrors++;
            printf("pathWithoutExtension('%s') => '%s' [%s]\n",
                   in.c_str(), out.c_str(),
                   ((out == expected) ? "ok" : "WRONG")
            );
        }
    }

    // test cases for function extensionOfPath()
    {
        // (a) input string, (b) expected result string
        std::pair<std::string, std::string> examples[] = {
            { "", "" },
            { "/foo/bla", "" },
            { "/foo", "" },
            { "/foo/asdf.gig", "gig" },
            { "/foo/asdf.gigx", "gigx" },
            { "foo", "" },
            { "asdf.gig", "gig" },
            { "asdf.blabla", "blabla" },
            { "C:/foo.gig", "gig"},
            { "C:/foo.blabla", "blabla"},
        };
        for (auto ex : examples) {
            const std::string in  = ex.first;
            const std::string out = extensionOfPath(in);
            const std::string expected = ex.second;
            if (out != expected) iErrors++;
            printf("extensionOfPath('%s') => '%s' [%s]\n",
                   in.c_str(), out.c_str(),
                   ((out == expected) ? "ok" : "WRONG")
            );
        }
    }

    printf("\n[There were %d errors]\n", iErrors);
}
