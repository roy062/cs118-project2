#!/bin/sh

for i in `shuf <<EOF
A
B
C
D
E
F
EOF`; do
    sleep 1
    if [ "$i" = "A" ]; then
        ./my-router A 10000 nodes.txt&
        echo "Node A listening on port 10000"
    elif [ "$i" = "B" ]; then
        ./my-router B 10001 nodes.txt&
        echo "Node B listening on port 10001"
    elif [ "$i" = "C" ]; then
        ./my-router C 10002 nodes.txt&
        echo "Node C listening on port 10002"
    elif [ "$i" = "D" ]; then
        ./my-router D 10003 nodes.txt&
        echo "Node D listening on port 10003"
    elif [ "$i" = "E" ]; then
        ./my-router E 10004 nodes.txt&
        echo "Node E listening on port 10004"
    elif [ "$i" = "F" ]; then
        ./my-router F 10005 nodes.txt&
        echo "Node F listening on port 10005"
    fi
done
