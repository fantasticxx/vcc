#!/bin/zsh

# Check if required compiler exists
if [ ! -f "./vcc" ]; then
    echo "❌ Error: compiler './vcc' not found"
    exit 1
fi

# Function to create a test file
create_test_file() {
    local file=$1
    local expression=$2
    
    cat > "$file" << EOF
main() {
    $expression;
}
EOF
}

# Function to run test and check result
run_test() {
    local test_num=$1
    local expression=$2
    local expected=$3
    local filename="testcase${test_num}.vc"
    
    echo "Test $test_num: $expression"
    create_test_file "$filename" "$expression"
    
    # Compile to assembly
    ./vcc "$filename"
    if [ ! -f "a.asm" ]; then
        echo "❌ Error: 'a.asm' was not generated for test $test_num"
        exit 1
    fi
    
    # Assemble
    nasm -f macho64 a.asm
    if [ ! -f "a.o" ]; then
        echo "❌ Error: 'a.o' was not generated for test $test_num"
        exit 1
    fi
    
    # Link
    gcc -m64 a.o
    if [ ! -f "a.out" ]; then
        echo "❌ Error: 'a.out' was not generated for test $test_num"
        exit 1
    fi
    
    # Run and get return value
    ./a.out
    result=$?
    
    if [ "$result" -eq "$expected" ]; then
        echo "✅ Test $test_num passed! Expected: $expected, Got: $result"
    else
        echo "❌ Test $test_num failed! Expected: $expected, Got: $result"
    fi
    echo "----------------------------------------"
    
    # Clean up files after each test
    rm -f a.asm a.o a.out
}

# Clean up any previous test files that might exist
rm -f testcase*.vc a.asm a.o a.out

# Run all test cases with corrected integer arithmetic
# Return value will be modulo 256 
run_test 1  "int a = (5 + 3) * 2"  16  
run_test 2  "int a = (10 - 7) * (4 + 2)"  18  
run_test 3  "int a = (8 / 2) + (6 - 3)"  7  
run_test 4  "int a = 20 / (2 + 3)"  4  
run_test 5  "int a = (15 - 4) + (10 / 5)"  13  

run_test 6  "int a = -((5 + 3) * 2)" 240
run_test 7  "int a = -(-(-10))" 246
run_test 8  "int a = (-4 * 3) + (10 / -2)" 239
run_test 9  "int a = (10 - (5 + 2)) * -3" 247
run_test 10 "int a = (-3) * (-2) * (-1)" 250

run_test 11 "int a = ((8 / 2) * 3) - (6 - 4)"  10
run_test 12 "int a = (3 + 2) * (7 - 4) / 5"  3
run_test 13 "int a = (20 / (4 + 1)) * (-2)" 248
run_test 14 "int a = ((3 * 2) + 4) / (-2)" 251
run_test 15 "int a = (-10 + (-5)) * (-2)"  30  

run_test 16 "int a = (100 / (2 * 5)) - (-4)"  14  
run_test 17 "int a = (((-3) + 7) * 2) - 5"  3  
run_test 18 "int a = (12 - 2 * 3) / (5 - 2)"  2  
run_test 19 "int a = -(((10 / 2) - 3) * 4)" 248
run_test 20 "int a = (7 - 3) * (2 + 4) - 5"  19  

run_test 21 "int a = 1 && 2 && 3 && 4 && (5 < 6) && (7 + 8) && (9 == 9)" 1
run_test 22 "int a = 'a' || 'b' && 1 || 0 && 1 || 123" 1
run_test 23 "int a = 1 || 0 && 1 || 0 && 1 || 0" 1
run_test 24 "int a = 0 && 0 || 0 && 0 || 0 || 0 || (0 && 1 && 2 && 3) || (0 || 0)" 0
# Final cleanup
rm -f testcase*.vc