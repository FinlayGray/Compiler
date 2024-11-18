; ModuleID = 'mini-c'
source_filename = "mini-c"

declare float @print_float(float)

define float @cosine(float %x) {
entry:
  %alt = alloca float, align 4
  %eps = alloca float, align 4
  %term = alloca float, align 4
  %n = alloca float, align 4
  %cos = alloca float, align 4
  %x1 = alloca float, align 4
  store float %x, ptr %x1, align 4
  %load_temp_float = load float, ptr %eps, align 4
  store float 0x3EB0C6F7A0000000, ptr %eps, align 4
  %load_temp_float2 = load float, ptr %n, align 4
  store float 1.000000e+00, ptr %n, align 4
  %load_temp_float3 = load float, ptr %cos, align 4
  store float 1.000000e+00, ptr %cos, align 4
  %load_temp_float4 = load float, ptr %term, align 4
  store float 1.000000e+00, ptr %term, align 4
  %load_temp_float5 = load float, ptr %alt, align 4
  store float -1.000000e+00, ptr %alt, align 4
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %load_temp_float6 = load float, ptr %term, align 4
  %load_temp_float7 = load float, ptr %eps, align 4
  %fgt_tmp = fcmp ogt float %load_temp_float6, %load_temp_float7
  %while_cond8 = icmp ne i1 %fgt_tmp, false
  br i1 %while_cond8, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %load_temp_float9 = load float, ptr %term, align 4
  %load_temp_float10 = load float, ptr %term, align 4
  %load_temp_float11 = load float, ptr %x1, align 4
  %fmul_tmp = fmul float %load_temp_float10, %load_temp_float11
  %load_temp_float12 = load float, ptr %x1, align 4
  %fmul_tmp13 = fmul float %fmul_tmp, %load_temp_float12
  %load_temp_float14 = load float, ptr %n, align 4
  %fdiv_tmp = fdiv float %fmul_tmp13, %load_temp_float14
  %load_temp_float15 = load float, ptr %n, align 4
  %fadd_tmp = fadd float %load_temp_float15, 1.000000e+00
  %fdiv_tmp16 = fdiv float %fdiv_tmp, %fadd_tmp
  store float %fdiv_tmp16, ptr %term, align 4
  %load_temp_float17 = load float, ptr %cos, align 4
  %load_temp_float18 = load float, ptr %cos, align 4
  %load_temp_float19 = load float, ptr %alt, align 4
  %load_temp_float20 = load float, ptr %term, align 4
  %fmul_tmp21 = fmul float %load_temp_float19, %load_temp_float20
  %fadd_tmp22 = fadd float %load_temp_float18, %fmul_tmp21
  store float %fadd_tmp22, ptr %cos, align 4
  %load_temp_float23 = load float, ptr %alt, align 4
  %load_temp_float24 = load float, ptr %alt, align 4
  %fneg_temp = fneg float %load_temp_float24
  store float %fneg_temp, ptr %alt, align 4
  %load_temp_float25 = load float, ptr %n, align 4
  %load_temp_float26 = load float, ptr %n, align 4
  %fadd_tmp27 = fadd float %load_temp_float26, 2.000000e+00
  store float %fadd_tmp27, ptr %n, align 4
  br label %while_cond

while_end:                                        ; preds = %while_cond
  %load_temp_float28 = load float, ptr %cos, align 4
  %call_tmp = call float @print_float(float %load_temp_float28)
  %load_temp_float29 = load float, ptr %cos, align 4
  ret float %load_temp_float29
}
