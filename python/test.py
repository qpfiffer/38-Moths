#!/usr/bin/env python3
from greshunkel import Template, Context, GshklFilterFunc

# Taken from the greshunkel_test.c file
test_template =\
"""
<html>
    <body>
        xXx SCREAM _include.html xXx
        xXx LOOP i LOOP_TEST xXx
            <li>xXx @TEST xXx xXx @i xXx</li>
        xXx BBL xXx
        <span>This is the real xXx @TRICKY xXx xXx @ONE xXx</span>
        <p>This is a regular string: xXx @TEST xXx</p>
        <p>This is an integer: xXx @FAKEINT xXx</p>
        <ul>
        xXx LOOP i LOOP_TEST xXx
            <li>XxX return_z xXx @i xXx XxX</li>
        xXx BBL xXx
        </ul>
        <p>Context Interpolation:</p>
        <p>xXx @sub.name xXx - xXx @sub.other xXx</p>
        <p>XxX return_hello doesnt_matter_at_all XxX</p>
        xXx LOOP subs SUB_LOOP_TEST xXx
            <p>FILTERS IN FILTERS IN LOOPS: XxX return_z F XxX</p>
            <p>XxX return_hello f XxX</p>
            <p>xXx @subs.name xXx - xXx @subs.other xXx</p>
        xXx BBL xXx
    </body>
</html>
"""


def test(arg):
    return "test".encode()

def main():
    test_func = GshklFilterFunc(test)
    context = Context({
        "TEST": "This is a test.",
        "FAKEINT": 666,
        "TRICKY": "TrIcKy",
        "ONE": 1,
        "LOOP_TEST": ["a", "b", "c", 1, 2, 3],
        "return_hello": test_func,
        "return_z": test_func
    })
    template = Template(test_template)
    print(template.render(context))

if __name__ == '__main__':
    main()
