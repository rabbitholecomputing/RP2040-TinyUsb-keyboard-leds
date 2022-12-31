# Test firmware for changing USB keyboard LEDs
This is a PlatformIO project to test `tuh_hid_set_report` failing after multiple calls successful calls.

The program waits for the user to press Caps Lock on the keyboard then toggles the LEDs of the keyboard automatically
until `tuh_set_report` fails.

## Compilation note
Line 69 of `tinyusb\src\common\tusb_debug.h` needs to be changed from 
`#define TU_LOG_PTR(n, ...)    TU_XSTRCAT3(TU_LOG, n, _PTR)(__VA_ARGS__)`

to 

`#define TU_LOG_PTR(n, ...)    TU_STRCAT3(TU_LOG, n, _PTR)(__VA_ARGS__)`

The gnu compiler has defined _PTR as (void *) which cause a
macro expansion error when TU_XSTRCAT3 is used. Using TU_STRCAT3 fixes the issue.