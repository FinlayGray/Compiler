; ModuleID = 'tests/addition/addition.c'
source_filename = "tests/addition/addition.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone
define dso_local i32 @addition(i32 noundef %n, i32 noundef %m) #0 {
entry:
  %n.addr = alloca i32, align 4
  %m.addr = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 %n, ptr %n.addr, align 4
  store i32 %m, ptr %m.addr, align 4
  %0 = load i32, ptr %n.addr, align 4
  %1 = load i32, ptr %m.addr, align 4
  %add = add nsw i32 %0, %1
  store i32 %add, ptr %result, align 4
  %2 = load i32, ptr %n.addr, align 4
  %cmp = icmp eq i32 %2, 4
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %3 = load i32, ptr %n.addr, align 4
  %4 = load i32, ptr %m.addr, align 4
  %add1 = add nsw i32 %3, %4
  %call = call i32 @print_int(i32 noundef %add1)
  br label %if.end

if.else:                                          ; preds = %entry
  %5 = load i32, ptr %n.addr, align 4
  %6 = load i32, ptr %m.addr, align 4
  %mul = mul nsw i32 %5, %6
  %call2 = call i32 @print_int(i32 noundef %mul)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %7 = load i32, ptr %result, align 4
  ret i32 %7
}

declare i32 @print_int(i32 noundef) #1

attributes #0 = { noinline nounwind optnone "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+cx8,+mmx,+sse,+sse2,+x87" }
attributes #1 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+cx8,+mmx,+sse,+sse2,+x87" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 18.1.8"}
