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
  store i32 %add_tmp, ptr %result, align 4
  %load_temp_int5 = load i32, ptr %n1, align 4
  %eq_tmp = icmp eq i32 %load_temp_int5, 4
  %if_cond = icmp ne i1 %eq_tmp, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %entry
  %load_temp_int6 = load i32, ptr %n1, align 4
  %load_temp_int7 = load i32, ptr %m2, align 4
  %add_tmp8 = add i32 %load_temp_int6, %load_temp_int7
  %call_tmp = call i32 @print_int(i32 %add_tmp8)
  br label %if_end

if_else:                                          ; preds = %entry
  %load_temp_int9 = load i32, ptr %n1, align 4
  %load_temp_int10 = load i32, ptr %m2, align 4
  %mul_tmp = mul i32 %load_temp_int9, %load_temp_int10
  %call_tmp11 = call i32 @print_int(i32 %mul_tmp)
  br label %if_end

if_end:                                           ; preds = %if_else, %if_then
  %load_temp_int12 = load i32, ptr %result, align 4
  ret i32 %load_temp_int12
}
