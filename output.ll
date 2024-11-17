; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @foo(i32 %x, i32 %y) {
entry:
  %y2 = alloca i32, align 4
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  store i32 %y, ptr %y2, align 4
  ret float 1.000000e+00
}
