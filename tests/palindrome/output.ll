; ModuleID = 'mini-c'
source_filename = "mini-c"

define i1 @palindrome(i32 %number) {
entry:
  %result = alloca i1, align 1
  %rmndr = alloca i32, align 4
  %rev = alloca i32, align 4
  %t = alloca i32, align 4
  %number1 = alloca i32, align 4
  store i32 %number, ptr %number1, align 4
  %load_temp_int = load i32, ptr %rev, align 4
  store i32 0, ptr %rev, align 4
  %load_temp_bool = load i1, ptr %result, align 1
  store i1 false, ptr %result, align 1
  %load_temp_int2 = load i32, ptr %t, align 4
  %load_temp_int3 = load i32, ptr %number1, align 4
  store i32 %load_temp_int3, ptr %t, align 4
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %load_temp_int4 = load i32, ptr %number1, align 4
  %gt_tmp = icmp sgt i32 %load_temp_int4, 0
  %while_cond5 = icmp ne i1 %gt_tmp, false
  br i1 %while_cond5, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_int6 = load i32, ptr %rmndr, align 4
  %load_temp_int7 = load i32, ptr %number1, align 4
  %mod_tmp = srem i32 %load_temp_int7, 10
  store i32 %mod_tmp, ptr %rmndr, align 4
  %load_temp_int8 = load i32, ptr %rev, align 4
  %load_temp_int9 = load i32, ptr %rev, align 4
  %mul_tmp = mul i32 %load_temp_int9, 10
  %load_temp_int10 = load i32, ptr %rmndr, align 4
  %add_tmp = add i32 %mul_tmp, %load_temp_int10
  store i32 %add_tmp, ptr %rev, align 4
  %load_temp_int11 = load i32, ptr %number1, align 4
  %load_temp_int12 = load i32, ptr %number1, align 4
  %div_tmp = sdiv i32 %load_temp_int12, 10
  store i32 %div_tmp, ptr %number1, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_int13 = load i32, ptr %t, align 4
  %load_temp_int14 = load i32, ptr %rev, align 4
  %eq_tmp = icmp eq i32 %load_temp_int13, %load_temp_int14
  %if_cond = icmp ne i1 %eq_tmp, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %while_end
  %load_temp_bool15 = load i1, ptr %result, align 1
  store i1 true, ptr %result, align 1
  br label %if_end

if_else:                                          ; preds = %while_end
  %load_temp_bool16 = load i1, ptr %result, align 1
  store i1 false, ptr %result, align 1
  br label %if_end

if_end:                                           ; preds = %if_else, %if_then
  %load_temp_bool17 = load i1, ptr %result, align 1
  ret i1 %load_temp_bool17
}
