; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addition(i32 %n, i32 %m) {
entry:
  %result = alloca i32, align 4
  %m2 = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 %m, ptr %m2, align 4
  %load_temp_int = load i32, ptr %result, align 4
  %load_temp_int3 = load i32, ptr %n1, align 4
  %load_temp_int4 = load i32, ptr %m2, align 4
  %add_tmp = add i32 %load_temp_int3, %load_temp_int4
}
