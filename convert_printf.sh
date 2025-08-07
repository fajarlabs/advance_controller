#!/bin/bash

# Script untuk mengganti semua printf dengan DEBUG_PRINTLN di project ESP32
# Dijalankan dari root project directory

echo "Converting printf to DEBUG_PRINTLN in all source files..."

# Array of directories to process
DIRS=(
    "components/ui"
    "main"
)

# Array of file extensions to process
EXTENSIONS=("*.c" "*.cpp" "*.h")

# Function to process a single file
process_file() {
    local file="$1"
    echo "Processing: $file"
    
    # Add my_global_lib.h include if not already present and if it's a .c file
    if [[ "$file" == *.c ]] && ! grep -q '#include "my_global_lib.h"' "$file"; then
        # Find the last #include line and add after it
        sed -i '/^#include.*$/a #include "my_global_lib.h"' "$file"
    fi
    
    # Replace printf patterns with DEBUG_PRINTLN
    # Handle printf with \n at the end
    sed -i 's/printf("\(.*\)\\n")/DEBUG_PRINTLN("\1")/g' "$file"
    
    # Handle printf with \n and parameters
    sed -i 's/printf("\(.*\)\\n", \(.*\))/DEBUG_PRINTLN("\1", \2)/g' "$file"
    
    # Handle printf without \n at the end
    sed -i 's/printf("\(.*\)")/DEBUG_PRINT("\1")/g' "$file"
    
    # Handle printf without \n but with parameters
    sed -i 's/printf("\(.*\)", \(.*\))/DEBUG_PRINT("\1", \2)/g' "$file"
}

# Process all files in specified directories
for dir in "${DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "Processing directory: $dir"
        for ext in "${EXTENSIONS[@]}"; do
            find "$dir" -name "$ext" -type f | while read -r file; do
                # Skip certain files that shouldn't be modified
                if [[ "$file" != *"lvgl"* ]] && [[ "$file" != *"managed_components"* ]]; then
                    process_file "$file"
                fi
            done
        done
    else
        echo "Directory not found: $dir"
    fi
done

echo "Conversion completed!"
echo ""
echo "Summary of changes:"
echo "- All printf() calls converted to DEBUG_PRINTLN() or DEBUG_PRINT()"
echo "- Added #include \"my_global_lib.h\" to .c files that need it"
echo ""
echo "To disable debug prints in production:"
echo "Set DEBUG_MODE to 0 in my_global_lib.h"
