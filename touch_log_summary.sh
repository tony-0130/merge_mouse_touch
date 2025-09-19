#!/bin/bash

# Pretty Touch Event Logger
# Reads from log.txt in the same folder

input_source="log.txt"

if [ ! -f "$input_source" ]; then
    echo "Error: log.txt not found!"
    exit 1
fi

touch_count=0

while IFS= read -r line; do
    if echo "$line" | grep -q "BTN_TOUCH.*value 1"; then
        ((touch_count++))
    fi
done < "$input_source"

echo "====================="
echo "Total touches: $touch_count"
cho "====================="
