; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = common global i32 0, align 4
@f = common global float 0.000000e+00, align 4
@b = common global i1 false, align 1

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %load_global_temp_float = load i32, ptr @test, align 4
  store i32 12, ptr @test, align 4
  %load_temp_int = load i32, ptr %result, align 4
  store i32 0, ptr %result, align 4
  %load_global_temp_float2 = load i32, ptr @test, align 4
  %call_tmp = call i32 @print_int(i32 %load_global_temp_float2)
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %load_temp_int3 = load i32, ptr %result, align 4
  %lt_tmp = icmp slt i32 %load_temp_int3, 10
  %while_cond4 = icmp ne i1 %lt_tmp, false
  br i1 %while_cond4, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_int5 = load i32, ptr %result, align 4
  %load_temp_int6 = load i32, ptr %result, align 4
  %add_tmp = add i32 %load_temp_int6, 1
  store i32 %add_tmp, ptr %result, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_int7 = load i32, ptr %result, align 4
  ret i32 %load_temp_int7
}
