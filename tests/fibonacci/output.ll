; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @fibonacci(i32 %n) {
entry:
  %total = alloca i32, align 4
  %c = alloca i32, align 4
  %next = alloca i32, align 4
  %second = alloca i32, align 4
  %first = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %load_temp_int = load i32, ptr %n1, align 4
  %call_tmp = call i32 @print_int(i32 %load_temp_int)
  %load_temp_int2 = load i32, ptr %first, align 4
  store i32 0, ptr %first, align 4
  %load_temp_int3 = load i32, ptr %second, align 4
  store i32 1, ptr %second, align 4
  %load_temp_int4 = load i32, ptr %c, align 4
  store i32 1, ptr %c, align 4
  %load_temp_int5 = load i32, ptr %total, align 4
  store i32 0, ptr %total, align 4
  br label %while_cond

while_cond:                                       ; preds = %if_end, %entry
  %load_temp_int6 = load i32, ptr %c, align 4
  %load_temp_int7 = load i32, ptr %n1, align 4
  %lt_tmp = icmp slt i32 %load_temp_int6, %load_temp_int7
  %while_cond8 = icmp ne i1 %lt_tmp, false
  br i1 %while_cond8, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_int9 = load i32, ptr %c, align 4
  %le_tmp = icmp sle i32 %load_temp_int9, 1
  %if_cond = icmp ne i1 %le_tmp, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %while_body
  %load_temp_int10 = load i32, ptr %next, align 4
  %load_temp_int11 = load i32, ptr %c, align 4
  store i32 %load_temp_int11, ptr %next, align 4
  br label %if_end

if_else:                                          ; preds = %while_body
  %load_temp_int12 = load i32, ptr %next, align 4
  %load_temp_int13 = load i32, ptr %first, align 4
  %load_temp_int14 = load i32, ptr %second, align 4
  %add_tmp = add i32 %load_temp_int13, %load_temp_int14
  store i32 %add_tmp, ptr %next, align 4
  %load_temp_int15 = load i32, ptr %first, align 4
  %load_temp_int16 = load i32, ptr %second, align 4
  store i32 %load_temp_int16, ptr %first, align 4
  %load_temp_int17 = load i32, ptr %second, align 4
  %load_temp_int18 = load i32, ptr %next, align 4
  store i32 %load_temp_int18, ptr %second, align 4
  br label %if_end

if_end:                                           ; preds = %if_else, %if_then
  %load_temp_int19 = load i32, ptr %next, align 4
  %call_tmp20 = call i32 @print_int(i32 %load_temp_int19)
  %load_temp_int21 = load i32, ptr %c, align 4
  %load_temp_int22 = load i32, ptr %c, align 4
  %add_tmp23 = add i32 %load_temp_int22, 1
  store i32 %add_tmp23, ptr %c, align 4
  %load_temp_int24 = load i32, ptr %total, align 4
  %load_temp_int25 = load i32, ptr %total, align 4
  %load_temp_int26 = load i32, ptr %next, align 4
  %add_tmp27 = add i32 %load_temp_int25, %load_temp_int26
  store i32 %add_tmp27, ptr %total, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_int28 = load i32, ptr %total, align 4
  %call_tmp29 = call i32 @print_int(i32 %load_temp_int28)
  %load_temp_int30 = load i32, ptr %total, align 4
  ret i32 %load_temp_int30
}
