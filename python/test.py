#!/usr/bin/env python3

# Taken from the greshunkel_test.c file
test_template = 
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


def main():

if __name__ == '__main__':
    main()
