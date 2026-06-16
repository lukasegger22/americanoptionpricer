# American Option Pricer

A small C++ project for pricing American-style options with the
Cox-Ross-Rubinstein binomial model.

The project is intentionally kept simple and presentation-friendly. The main
goal is to show the mathematical idea, the use of C++ containers and functions,
and a small data workflow with CSV input and an optional API update.

## What The Program Does

The program prices options by building a binomial stock price tree. At each time
step the stock price can move up or down. At maturity, the option payoff is
known. The program then works backwards through the tree.

For an American option, each node compares:

- continuation value: keep the option
- exercise value: exercise the option immediately

The American value is:

```text
max(continuation value, exercise value)
```

For comparison, the program also computes the European value, where early
exercise is not allowed. The difference is the early exercise premium.

## Features

- American and European option pricing
- Call and put payoff support
- Cox-Ross-Rubinstein binomial tree
- Backward induction
- Early exercise premium
- Different binomial step counts for comparison
- Volatility analysis
- CSV input with `std::vector<Option>`
- Optional Alpha Vantage market data update
- Fallback to local CSV data if the API is unavailable
- Small terminal tree preview
- SVG output of a 3-step binomial tree

## Project Structure

```text
.
├── CMakeLists.txt
├── README.md
├── run.sh
├── fetch_data.sh
├── data
│   └── options.csv
├── docs
│   ├── CODE_WALKTHROUGH.md
│   └── presentation.tex
└── src
    └── main.cpp
```

## Input Data

The option examples are stored in:

```text
data/options.csv
```

CSV format:

```csv
name,spot,strike,rate,volatility,maturity,type
AAPL Put Example,291.00,300.00,0.045,0.25,1.0,PUT
AAPL Call Example,291.00,300.00,0.045,0.25,1.0,CALL
High Volatility Put,100.00,100.00,0.045,0.40,1.0,PUT
```

The C++ program reads these rows into a `std::vector<Option>`.

## Market Data

The program can optionally update the selected stock spot price with Alpha
Vantage daily stock data. The current symbol in the C++ code is `AAPL`.

Current workflow:

```text
./run.sh
```

If `.alphavantage_key` exists, the program automatically tries to fetch the
latest close price for the selected symbol. If the API call fails, it still runs
with the fallback values from `data/options.csv`.

Safe presentation behavior:

```text
API works  -> use fresh market close price as spot
API fails  -> use values from data/options.csv
No API key -> use values from data/options.csv
```

Common symbols that are easy to test with Alpha Vantage:

```text
AAPL  Apple
MSFT  Microsoft
GOOGL Alphabet / Google
AMZN  Amazon
TSLA  Tesla
NVDA  Nvidia
```

To test another symbol, change `marketSymbol` in `src/main.cpp` and make sure
the option names in `data/options.csv` contain the same symbol, for example
`MSFT Call Example`.

For current market data, use a personal free Alpha Vantage API key. Without an
API key, the program automatically uses the fallback values from `data/options.csv`.

The fetch script checks for the API key in this order:

1. environment variable `ALPHAVANTAGE_API_KEY`
2. local file `.alphavantage_key`
3. fallback to `data/options.csv` if no key or no valid API response is available

The file `.alphavantage_key` is ignored by Git, so the API key is not uploaded
to GitHub.

## Build And Run

Simple run:

```bash
./run.sh
```

Run with current Alpha Vantage market data:

```bash
ALPHAVANTAGE_API_KEY=your_api_key_here ./run.sh
```

Alternatively, store your API key locally in a file that is ignored by Git:

```bash
echo "your_api_key_here" > .alphavantage_key
./run.sh
```

With this setup, `./run.sh` automatically calls the API update before pricing.

Manual build:

```bash
cmake -S . -B build
cmake --build build
./build/AmericanOptionPricer
```

Manual API fetch:

```bash
ALPHAVANTAGE_API_KEY=your_api_key_here ./fetch_data.sh AAPL
```

## Output

The program prints:

- American and European prices
- early exercise premium
- results for several step counts
- a 3-step tree preview in the terminal
- a volatility analysis
- a short summary

The 25, 50, 100, and 200 step calculations are used for the actual pricing
comparison. The 3-step tree is only a small visualization, because a 100-step or
200-step tree would not be readable.

It also creates:

```text
binomial_tree.svg
```

This file shows a small visual binomial tree. Each node contains:

```text
S = stock price
V = American option value
```

## C++ Features Used

- `struct` for option data
- `enum` for option type and exercise style
- `std::vector` for option lists and tree values
- `std::string` for names and CSV fields
- file input with `std::ifstream`
- file output with `std::ofstream`
- string parsing with `std::stringstream`
- math functions from `<cmath>`
- formatting with `<iomanip>`
- simple error handling with validation and fallback logic
- shell script integration for optional API data

Main examples:

| Feature | Used for |
| --- | --- |
| `std::vector<Option>` | stores all option contracts from CSV |
| `std::vector<double>` | stores one level of binomial tree option values |
| `std::vector<std::vector<double>>` | stores the small 3-step visualization tree |
| `std::ifstream` | reads CSV option data and latest API price |
| `std::ofstream` | writes `binomial_tree.svg` |
| `std::stringstream` | parses CSV rows |
| `std::max` | computes payoffs and American early exercise decision |
| `std::exp`, `std::sqrt`, `std::pow` | implements the CRR model formulas |

Features intentionally not used:

- threads: not needed because the computation is small and fast
- `std::optional`: simple `bool` return values are enough here
- `std::variant`: the option data has one fixed structure
- external JSON library: API output is reduced to CSV before C++ reads it

## Model Assumptions

This is a simplified educational pricing model.

- no dividends
- constant volatility
- constant risk-free rate
- no transaction costs
- no bid-ask spread
- no real option market calibration
- API updates only the stock spot price

These assumptions keep the project small enough to understand and present, while
still showing the main mathematical idea behind American option pricing.

## Further Documentation

Detailed code explanation:

```text
docs/CODE_WALKTHROUGH.md
```

Short Beamer presentation:

```text
docs/presentation.tex
```

Compile the presentation:

```bash
pdflatex -output-directory docs docs/presentation.tex
```
