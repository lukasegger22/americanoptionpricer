#!/bin/sh

SYMBOL=${1:-AAPL}
PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
KEY_FILE="$PROJECT_DIR/.alphavantage_key"
OUTPUT_DIR="$PROJECT_DIR/data"
DAILY_FILE="$OUTPUT_DIR/${SYMBOL}_daily.csv"
LATEST_FILE="$OUTPUT_DIR/${SYMBOL}_latest_price.csv"
TEMP_FILE="$OUTPUT_DIR/${SYMBOL}_daily.tmp.csv"

if [ -z "$ALPHAVANTAGE_API_KEY" ] && [ -f "$KEY_FILE" ]; then
    ALPHAVANTAGE_API_KEY=$(sed -n '1p' "$KEY_FILE" | tr -d '[:space:]')
fi

if [ -z "$ALPHAVANTAGE_API_KEY" ]; then
    echo "No API key found."
    echo "Either run: export ALPHAVANTAGE_API_KEY=your_api_key_here"
    echo "Or create: $KEY_FILE"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

curl -s "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=$SYMBOL&apikey=$ALPHAVANTAGE_API_KEY&datatype=csv" -o "$TEMP_FILE"

if ! head -n 1 "$TEMP_FILE" | grep -q "timestamp,open,high,low,close,volume"; then
    rm -f "$TEMP_FILE"
    echo "Could not fetch valid market data for $SYMBOL."
    exit 1
fi

mv "$TEMP_FILE" "$DAILY_FILE"

awk -F, -v symbol="$SYMBOL" 'NR==1 {next} NR==2 {print "symbol,date,close"; print symbol "," $1 "," $5}' "$DAILY_FILE" > "$LATEST_FILE"

echo "Market data saved to $LATEST_FILE"
