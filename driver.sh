#!/bin/zsh

# Check if required compiler exists
if [ ! -f "./vcc" ]; then
    echo "❌ Error: compiler './vcc' not found"
    exit 1
fi
# Function to run a test file
run_test() {
    local file=$1
    local expected=$2
    local expected_output=$3
    echo "Running test: $file"
    echo "----------------------------------------"
    
    if [ ! -f "$file" ]; then
        echo "❌ Error: Test file $file not found"
        echo "----------------------------------------"
        return 1
    fi

    # Compile to assembly
    ./vcc "$file"
    if [ ! -f "a.asm" ]; then
        echo "❌ Error: 'a.asm' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi
    
    # Assemble
    nasm -f macho64 a.asm
    if [ ! -f "a.o" ]; then
        echo "❌ Error: 'a.o' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi
    
    # Link
    gcc -m64 a.o
    if [ ! -f "a.out" ]; then
        echo "❌ Error: 'a.out' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi
    
    local tmp_file=$(mktemp)
    ./a.out > "$tmp_file"
    local exec_result=$?
    local actual_output=$(cat "$tmp_file")
    rm -f "$tmp_file"
    diff_output=$(diff <(echo "$actual_output") <(echo "$expected_output") 2>/dev/null)
    comparison_result=$?
    if [ "$exec_result" -eq "$expected" ] && [ "$comparison_result" -eq 0 ]; then
        echo "✅ Test $file passed! Expected: $expected, Got: $exec_result"
    else
        echo "❌ Test $file failed! Expected: $expected, Got: $exec_result"
        echo "Expected:\n $expected_output"
        echo "Actual:\n $actual_output"
    fi
    echo "----------------------------------------"
    
    # Clean up files
    rm -f a.asm a.o a.out
}

run_test_fail() {
    local file=$1
    local expected_error_msg=$2
    echo "Running test: $file"
    echo "----------------------------------------"
    local tmp_file=$(mktemp)
    ./vcc "$file" 2> "$tmp_file"
    local compile_result=$?
    local actual_error_msg=$(cat "$tmp_file")
    rm -f "$tmp_file"
    if [ $compile_result -eq 0 ]; then
        echo "❌ Test $file should have failed but succeeded"
        echo "----------------------------------------"
        return 1
    fi
    if diff <(echo "$actual_error_msg") <(echo "$expected_error_msg") > /dev/null; then
        echo "✅ Test $file passed! Error message matches expected output"
        echo "----------------------------------------"
    else
        echo "❌ Test $file passed! Error message does not match expected output"
        echo "Expected:\n $expected_error_msg"
        echo "Actual:\n $actual_error_msg"
        echo "----------------------------------------"
        return 1
    fi

    rm -f a.asm a.o a.out
    return 0
}

run_test_input() {
    local file=$1
    local expected=$2
    local expected_output=$3
    local input1=$4
    local input2=$5
    echo "Running test: $file"
    echo "----------------------------------------"

    if [ ! -f "$file" ]; then
        echo "❌ Error: Test file $file not found"
        return 1
    fi

    # Compile to assembly
    ./vcc "$file"
    if [ ! -f "a.asm" ]; then
        echo "❌ Error: 'a.asm' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi

    # Assemble
    nasm -f macho64 a.asm
    if [ ! -f "a.o" ]; then
        echo "❌ Error: 'a.o' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi
    
    # Link
    gcc -m64 a.o
    if [ ! -f "a.out" ]; then
        echo "❌ Error: 'a.out' was not generated for $file"
        echo "----------------------------------------"
        return 1
    fi

    local tmp_file=$(mktemp)
    echo "$input1" "$input2" | ./a.out > "$tmp_file"
    local exec_result=$?
    local actual_output=$(cat "$tmp_file")
    rm -f "$tmp_file"
    diff_output=$(diff <(echo "$actual_output") <(echo "$expected_output") 2>/dev/null)
    comparison_result=$?
    if [ "$exec_result" -eq "$expected" ] && [ "$comparison_result" -eq 0 ]; then
        echo "✅ Test $file passed! Expected: $expected, Got: $exec_result"
    else
        echo "❌ Test $file failed! Expected: $expected, Got: $exec_result"
        echo "Expected:\n $expected_output"
        echo "Actual:\n $actual_output"
    fi
    echo "----------------------------------------"
    rm -f a.asm a.o a.out
}

# Find and run all .vc files in test_program directory
run_test ./test_program/test_program-A1/vc-A1-T01.vc 0
run_test_fail ./test_program/test_program-A1/vc-A1-T02.vc "syntax error"
run_test_fail ./test_program/test_program-A1/vc-A1-T03.vc "15: Unexpected character: [\n15: Unexpected character: ]\nsyntax error"
run_test_fail ./test_program/test_program-A1/vc-A1-T04.vc "syntax error"
run_test ./test_program/test_program-A2/vc-A2-T01.vc 0
run_test_fail ./test_program/test_program-A2/vc-A2-T02.vc "parser: use of undeclared identifier 'def1'\nparser: use of undeclared identifier 'B77'\nparser: redefinition of y456"
run_test ./test_program/test_program-A2/vc-A2-T03.vc 0
run_test ./test_program/test_program-A3/vc-A3-T01.vc 0 "Hello, world!"
run_test ./test_program/test_program-A3/vc-A3-T02.vc 0 "Hello, world!"
name="Hank"
age=20
bornyear=$((2016-age+1))
run_test_input ./test_program/test_program-A3/vc-A3-T03.vc 0 "Hello, what is your name?\nHi, $name, please enter your age.\nYou were born in year$bornyear\nGood-bye." $name $age
K=20
A=$((K/4*2))
run_test_input ./test_program/test_program-A3/vc-A3-T04.vc 0 "Hello, program execution starts.\nPleade enter a number.\nThe half is$A\nGood-bye." $K
n1=100
n2=80
ans=$(( (n1+n2)*(n1-n2)+1 ))
run_test_input ./test_program/test_program-A3/vc-A3-T05.vc 0 "Hello, program execution starts.\nPleade enter the value of n1:\nPleade enter the value of n2:\nThe answer is$ans\nGood-bye." $n1 $n2
N=10
run_test_input ./test_program/test_program-A4/vc-A4-T01.vc 0 "Please enter N: // ****\nFibonacci($N) 89\n// ****" $N
run_test_input ./test_program/test_program-A4/vc-A4-T02.vc 0 "Please enter A: Please enter B: GCD=1" 13 17 