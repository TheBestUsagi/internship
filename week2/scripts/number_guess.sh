#!/usr/bin/env bash

low=1
high=100
target=$(( RANDOM % (high - low + 1) + low ))
attempts=0

# Ctrl+C 
trap 'echo; echo "quit.... the right answer is ：$target"; exit 0' INT

echo "（$low to $high）——type a number to begin，type q to quit。"

while true; do
  read -r -p "type your guess： " guess

  
  if [[ $guess == "q" || $guess == "Q" ]]; then
    echo "see ya the answer ：$target"
    exit 0
  fi

    if ! [[ $guess =~ ^[+-]?[0-9]+$ ]]; then
    echo " only integer allowed（or q to quit）。"
    continue
  fi
  if (( guess <= 0 )); then
    echo " your number should be bigger than 0。"
    continue
  fi

  ((attempts++))

  # 比较大小
  if (( guess < target )); then
    echo "too small ↑"
  elif (( guess > target )); then
    echo "too big   ↓"
  else
    echo "congratulations！answer is  $target（you take $attempts times）"
    break
  fi
done

