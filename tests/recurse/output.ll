; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addNumbers(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %load_temp_int = load i32, ptr %result, align 4
  store i32 0, ptr %result, align 4
  %load_temp_int2 = load i32, ptr %n1, align 4
  %ne_tmp = icmp ne i32 %load_temp_int2, 0
  %if_cond = icmp ne i1 %ne_tmp, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %entry
  %load_temp_int3 = load i32, ptr %result, align 4
  %load_temp_int4 = load i32, ptr %n1, align 4
  %load_temp_int5 = load i32, ptr %n1, align 4
  %sub_tmp = sub i32 %load_temp_int5, 1
  %call_tmp = call i32 @addNumbers(i32 %sub_tmp)
  %add_tmp = add i32 %load_temp_int4, %call_tmp
  store i32 %add_tmp, ptr %result, align 4
  br label %if_end

if_else:                                          ; preds = %entry
  %load_temp_int6 = load i32, ptr %result, align 4
  %load_temp_int7 = load i32, ptr %n1, align 4
  store i32 %load_temp_int7, ptr %result, align 4
  br label %if_end

if_end:                                           ; preds = %if_else, %if_then
  %load_temp_int8 = load i32, ptr %result, align 4
  %call_tmp9 = call i32 @print_int(i32 %load_temp_int8)
  %load_temp_int10 = load i32, ptr %result, align 4
  ret i32 %load_temp_int10
}

define i32 @recursion_driver(i32 %num) {
entry:
  %num1 = alloca i32, align 4
  store i32 %num, ptr %num1, align 4
  %load_temp_int = load i32, ptr %num1, align 4
  %call_tmp = call i32 @addNumbers(i32 %load_temp_int)
  ret i32 %call_tmp
}
