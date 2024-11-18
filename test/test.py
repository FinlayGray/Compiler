import os
from os import devnull
import sys
import shutil
import glob
import stat
import subprocess


def setup():
    tmpdir = "./" + "/".join(__file__.split("/")[:-1]) + "/tmp"

    if os.path.exists(tmpdir):
        shutil.rmtree(tmpdir)

    os.mkdir(tmpdir)

    # Build the script

    if len(sys.argv) <= 1:
        print("Please enter the location of the makefile")
        exit()

    current_dir = os.getcwd()
    makefilefolder = os.getcwd() + "/./" + "".join(sys.argv[1].split("/")[:-1])
    os.chdir(makefilefolder)

    print("Compiling...")
    with open(os.devnull, "wb") as devnull:
        subprocess.check_call(["make"], stdout=devnull, stderr=subprocess.STDOUT)
    print("Compiled!")

    tmpdir = os.path.join(current_dir, tmpdir)
    os.chdir(current_dir)

    mccomp_file = os.path.join(tmpdir + "/mccomp")
    shutil.copyfile(makefilefolder + "/mccomp", tmpdir + "/mccomp")
    shutil.copyfile(os.path.join(tmpdir, "../driver.cpp"), tmpdir + "/driver.cpp")
    st = os.stat(mccomp_file)
    os.chmod(mccomp_file, st.st_mode | stat.S_IEXEC)

    os.chdir(tmpdir)

    tests_dir = os.path.join(tmpdir, "../tests")
    all_passed = True

    for file in os.listdir(tests_dir):
        if file.endswith(".c"):
            file_path = os.path.join(tests_dir, "./{0}".format(file))
            results, should_gen = get_file_data(file_path)
            res = run_test(file_path, results, should_gen)
            case = ".".join(file.split(".")[:-1])
            if res == True:
                print("Passed test %s" % case)
            else:
                all_passed = False
                print("Failed test {0}. Reason: {1}".format(case, res))

    if all_passed:
        print("ALL PASSED")

    if os.path.exists(tmpdir):
        shutil.rmtree(tmpdir)


def get_file_data(file):
    data = open(file, "r")
    gens = True
    results = list()

    line = data.readline().strip()

    while line == "":
        line = data.readline().strip()

    if line == "// N":
        gens = False

    line = data.readline()

    while line:
        if not line.startswith("// "):
            break

        results.append(line.split("// ")[1].split("\n")[0])
        line = data.readline()

    return results, gens


def run_test(file, results, should_gen=True):
    try:
        with open(os.devnull, "wb") as devnull:
            subprocess.check_call(
                ["./mccomp", file], stdout=devnull, stderr=subprocess.STDOUT)
        if not should_gen:
            return "expected generation error but succeeded"
    except:
        if should_gen:
            return "failed to generate code"
        else:
            return True

    try:
        with open(os.devnull, "wb") as devnull:
            subprocess.check_call(["clang++", "driver.cpp", "output.ll", "-o",
                                   "out"], stdout=devnull, stderr=subprocess.STDOUT)
    except:
        return "failed to compile"

    output = subprocess.check_output(["./out"])
    outputs = output.decode("utf-8").split("\n")[:-1]

    if outputs != results:
        return "different results expected: {0} actual: {1}".format(str(results), str(outputs))

    return True


setup()
