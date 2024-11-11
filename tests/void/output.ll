; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define void @Void() {
entry:
  %result = alloca i32, align 4
  %load_temp_int = load i32, ptr %result, align 4
  store i32 0, ptr %result, align 4
  %load_temp_int1 = load i32, ptr %result, align 4
  %call_tmp = call i32 @print_int(i32 %load_temp_int1)
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %load_temp_int2 = load i32, ptr %result, align 4
  %lt_tmp = icmp slt i32 %load_temp_int2, 10
  %while_cond3 = icmp ne i1 %lt_tmp, false
  br i1 %while_cond3, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_int4 = load i32, ptr %result, align 4
  %load_temp_int5 = load i32, ptr %result, align 4
  %add_tmp = add i32 %load_temp_int5, 1
  store i32 %add_tmp, ptr %result, align 4
  %load_temp_int6 = load i32, ptr %result, align 4
  %call_tmp7 = call i32 @print_int(i32 %load_temp_int6)
  br label %while_cond

while_end:                                        ; preds = %while_cond
  ret void
}
