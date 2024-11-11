; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @pi() {
entry:
  %i = alloca i32, align 4
  %PI = alloca float, align 4
  %flag = alloca i1, align 1
  %load_temp_bool = load i1, ptr %flag, align 1
  store i1 true, ptr %flag, align 1
  %load_temp_float = load float, ptr %PI, align 4
  store float 3.000000e+00, ptr %PI, align 4
  %load_temp_int = load i32, ptr %i, align 4
  store i32 2, ptr %i, align 4
  br label %while_cond

while_cond:                                       ; preds = %if_end, %entry
  %load_temp_int1 = load i32, ptr %i, align 4
  %lt_tmp = icmp slt i32 %load_temp_int1, 100
  %while_cond2 = icmp ne i1 %lt_tmp, false
  br i1 %while_cond2, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_bool3 = load i1, ptr %flag, align 1
  %if_cond = icmp ne i1 %load_temp_bool3, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %while_body
  %load_temp_float4 = load float, ptr %PI, align 4
  %load_temp_float5 = load float, ptr %PI, align 4
  %load_temp_int6 = load i32, ptr %i, align 4
  %load_temp_int7 = load i32, ptr %i, align 4
  %add_tmp = add i32 %load_temp_int7, 1
  %mul_tmp = mul i32 %load_temp_int6, %add_tmp
  %load_temp_int8 = load i32, ptr %i, align 4
  %add_tmp9 = add i32 %load_temp_int8, 2
  %mul_tmp10 = mul i32 %mul_tmp, %add_tmp9
  %cast_to_float = sitofp i32 %mul_tmp10 to float
  %fdiv_tmp = fdiv float 4.000000e+00, %cast_to_float
  %fadd_tmp = fadd float %load_temp_float5, %fdiv_tmp
  store float %fadd_tmp, ptr %PI, align 4
  br label %if_end

if_else:                                          ; preds = %while_body
  %load_temp_float11 = load float, ptr %PI, align 4
  %load_temp_float12 = load float, ptr %PI, align 4
  %load_temp_int13 = load i32, ptr %i, align 4
  %load_temp_int14 = load i32, ptr %i, align 4
  %add_tmp15 = add i32 %load_temp_int14, 1
  %mul_tmp16 = mul i32 %load_temp_int13, %add_tmp15
  %load_temp_int17 = load i32, ptr %i, align 4
  %add_tmp18 = add i32 %load_temp_int17, 2
  %mul_tmp19 = mul i32 %mul_tmp16, %add_tmp18
  %cast_to_float20 = sitofp i32 %mul_tmp19 to float
  %fdiv_tmp21 = fdiv float 4.000000e+00, %cast_to_float20
  %fsub_tmp = fsub float %load_temp_float12, %fdiv_tmp21
  store float %fsub_tmp, ptr %PI, align 4
  br label %if_end

if_end:                                           ; preds = %if_else, %if_then
  %load_temp_bool22 = load i1, ptr %flag, align 1
  %load_temp_bool23 = load i1, ptr %flag, align 1
  %not_temp = xor i1 %load_temp_bool23, true
  store i1 %not_temp, ptr %flag, align 1
  %load_temp_int24 = load i32, ptr %i, align 4
  %load_temp_int25 = load i32, ptr %i, align 4
  %add_tmp26 = add i32 %load_temp_int25, 2
  store i32 %add_tmp26, ptr %i, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_float27 = load float, ptr %PI, align 4
  ret float %load_temp_float27
}
