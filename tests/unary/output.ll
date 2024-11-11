; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define float @unary(i32 %n, float %m) {
entry:
  %sum = alloca float, align 4
  %result = alloca float, align 4
  %m2 = alloca float, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store float %m, ptr %m2, align 4
  %load_temp_float = load float, ptr %sum, align 4
  store float 0.000000e+00, ptr %sum, align 4
  %load_temp_float3 = load float, ptr %result, align 4
  %load_temp_int = load i32, ptr %n1, align 4
  %load_temp_float4 = load float, ptr %m2, align 4
  %cast_to_float = sitofp i32 %load_temp_int to float
  %fadd_tmp = fadd float %cast_to_float, %load_temp_float4
  store float %fadd_tmp, ptr %result, align 4
  %load_temp_float5 = load float, ptr %result, align 4
  %call_tmp = call float @print_float(float %load_temp_float5)
  %load_temp_float6 = load float, ptr %sum, align 4
  %load_temp_float7 = load float, ptr %sum, align 4
  %load_temp_float8 = load float, ptr %result, align 4
  %fadd_tmp9 = fadd float %load_temp_float7, %load_temp_float8
  store float %fadd_tmp9, ptr %sum, align 4
  %load_temp_float10 = load float, ptr %result, align 4
  %load_temp_int11 = load i32, ptr %n1, align 4
  %load_temp_float12 = load float, ptr %m2, align 4
  %fneg_temp = fneg float %load_temp_float12
  %cast_to_float13 = sitofp i32 %load_temp_int11 to float
  %fadd_tmp14 = fadd float %cast_to_float13, %fneg_temp
  store float %fadd_tmp14, ptr %result, align 4
  %load_temp_float15 = load float, ptr %result, align 4
  %call_tmp16 = call float @print_float(float %load_temp_float15)
  %load_temp_float17 = load float, ptr %sum, align 4
  %load_temp_float18 = load float, ptr %sum, align 4
  %load_temp_float19 = load float, ptr %result, align 4
  %fadd_tmp20 = fadd float %load_temp_float18, %load_temp_float19
  store float %fadd_tmp20, ptr %sum, align 4
  %load_temp_float21 = load float, ptr %result, align 4
  %load_temp_int22 = load i32, ptr %n1, align 4
  %load_temp_float23 = load float, ptr %m2, align 4
  %fneg_temp24 = fneg float %load_temp_float23
  %fneg_temp25 = fneg float %fneg_temp24
  %cast_to_float26 = sitofp i32 %load_temp_int22 to float
  %fadd_tmp27 = fadd float %cast_to_float26, %fneg_temp25
  store float %fadd_tmp27, ptr %result, align 4
  %load_temp_float28 = load float, ptr %result, align 4
  %call_tmp29 = call float @print_float(float %load_temp_float28)
  %load_temp_float30 = load float, ptr %sum, align 4
  %load_temp_float31 = load float, ptr %sum, align 4
  %load_temp_float32 = load float, ptr %result, align 4
  %fadd_tmp33 = fadd float %load_temp_float31, %load_temp_float32
  store float %fadd_tmp33, ptr %sum, align 4
  %load_temp_float34 = load float, ptr %result, align 4
  %load_temp_int35 = load i32, ptr %n1, align 4
  %neg_temp = sub i32 0, %load_temp_int35
  %load_temp_float36 = load float, ptr %m2, align 4
  %fneg_temp37 = fneg float %load_temp_float36
  %cast_to_float38 = sitofp i32 %neg_temp to float
  %fadd_tmp39 = fadd float %cast_to_float38, %fneg_temp37
  store float %fadd_tmp39, ptr %result, align 4
  %load_temp_float40 = load float, ptr %result, align 4
  %call_tmp41 = call float @print_float(float %load_temp_float40)
  %load_temp_float42 = load float, ptr %sum, align 4
  %load_temp_float43 = load float, ptr %sum, align 4
  %load_temp_float44 = load float, ptr %result, align 4
  %fadd_tmp45 = fadd float %load_temp_float43, %load_temp_float44
  store float %fadd_tmp45, ptr %sum, align 4
  %load_temp_float46 = load float, ptr %sum, align 4
  ret float %load_temp_float46
}
