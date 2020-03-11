GRESHUNKEL uses the `UNLESS` and `UNLESS NOT` operators from Ruby, which are
objectively easier to think about than `if` and `if not`. If you're having
trouble thinking about which is which because you grew up writing C, then just
think of them as `if` statements with an extra inversion, so `unless` becomes
`if not` and `unless not` becomes `if not not` becomes `if`. Get it? Easy!

```HTML
xXx UNLESS @user xXx
    No user.
xXx ENDLESS xXx
```

This will display `No user.` if the `@user` variable is not set in the
`greshunkel_context`. Heres a rough guide to truthy/falsey statements in
GRESHUNKEL. There does not exist a boolean type, so good luck.

```
subcontexts = True
integers = False
strings = True as long as they aren't the string FALSE
arrays = False
```
