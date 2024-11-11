; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @factorial(i32 %n) {
entry:
  %factorial = alloca i32, align 4
  %i = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %load_temp_int = load i32, ptr %factorial, align 4
  store i32 1, ptr %factorial, align 4
  %load_temp_int2 = load i32, ptr %i, align 4
  store i32 1, ptr %i, align 4
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %load_temp_int3 = load i32, ptr %i, align 4
  %load_temp_int4 = load i32, ptr %n1, align 4
  %le_tmp = icmp sle i32 %load_temp_int3, %load_temp_int4
  %while_cond5 = icmp ne i1 %le_tmp, false
  br i1 %while_cond5, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_int6 = load i32, ptr %factorial, align 4
  %load_temp_int7 = load i32, ptr %factorial, align 4
  %load_temp_int8 = load i32, ptr %i, align 4
  %mul_tmp = mul i32 %load_temp_int7, %load_temp_int8
  store i32 %mul_tmp, ptr %factorial, align 4
  %load_temp_int9 = load i32, ptr %i, align 4
  %load_temp_int10 = load i32, ptr %i, align 4
  %add_tmp = add i32 %load_temp_int10, 1
  store i32 %add_tmp, ptr %i, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_int11 = load i32, ptr %factorial, align 4
  ret i32 %load_temp_int11
}
