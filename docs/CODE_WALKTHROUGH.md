# Code Walkthrough

This document explains the source code in `src/main.cpp` in detail. It is
written as preparation material for the oral presentation.

## Overall Program Idea

The program prices options with a binomial tree.

The financial input parameters are:

- `spot`: current stock price, written as `S_0`
- `strike`: exercise price, written as `K`
- `rate`: risk-free interest rate, written as `r`
- `volatility`: stock volatility, written as `sigma`
- `maturity`: time to maturity in years, written as `T`
- `type`: call or put

The binomial model assumes that the stock price can move up or down in each
time step. At the end of the tree, the payoff is known. The program then uses
backward induction to calculate the option value at the beginning.

For American options, early exercise is allowed. Therefore, at each node the
program compares:

```text
continuation value = discounted expected future value
exercise value     = payoff if exercised immediately
```

The American option value is:

```text
max(continuation value, exercise value)
```

## Included Libraries

```cpp
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
```

`<algorithm>` is used for `std::max`.

`<cctype>` is used for `std::toupper` when parsing option types from CSV.

`<cmath>` is used for mathematical functions such as `std::exp`,
`std::sqrt`, and `std::pow`.

`<cstdlib>` is used for `std::system`, which calls the optional API update
script `fetch_data.sh`.

`<fstream>` is used for reading CSV files and writing the SVG file.

`<iomanip>` is used for formatted terminal and SVG numeric output.

`<iostream>` is used for terminal output with `std::cout`.

`<sstream>` is used to split CSV lines.

`<string>` is used for names, file paths, and CSV text.

`<vector>` is used for dynamic containers: option lists, tree levels, and
analysis parameter lists.

## OptionType

```cpp
enum OptionType {
    CALL,
    PUT
};
```

This enum represents the two option payoff types.

For a call option:

```text
payoff = max(S - K, 0)
```

For a put option:

```text
payoff = max(K - S, 0)
```

Using an enum is clearer than using strings like `"CALL"` or `"PUT"`
everywhere in the pricing logic.

## ExerciseStyle

```cpp
enum ExerciseStyle {
    EUROPEAN,
    AMERICAN
};
```

This enum distinguishes the exercise rule.

European options can only be exercised at maturity.

American options can be exercised at every node before maturity.

The same pricing function can price both styles by checking this enum.

## Option Struct

```cpp
struct Option {
    std::string name;
    double spot;
    double strike;
    double rate;
    double volatility;
    double maturity;
    OptionType type;
};
```

The `Option` struct stores all input parameters for one option.

This is a simple data container. It is easy to explain and avoids spreading
many separate variables through the code.

In the project, multiple `Option` objects are stored in:

```cpp
std::vector<Option>
```

This directly addresses the C++ container requirement.

## payoff

```cpp
double payoff(double stockPrice, double strike, OptionType type)
```

This function computes the intrinsic value of the option.

If the option is a call:

```cpp
return std::max(stockPrice - strike, 0.0);
```

If the option is a put:

```cpp
return std::max(strike - stockPrice, 0.0);
```

`std::max` ensures that the payoff can never be negative.

This function is used in two places:

- at maturity, to initialize the final option values
- at earlier nodes, to check whether early exercise is better

## optionTypeName

```cpp
std::string optionTypeName(OptionType type)
```

This helper converts the enum into text for printing.

If the type is `CALL`, it returns `"Call"`.

Otherwise, it returns `"Put"`.

This function is only for output formatting.

## parseOptionType

```cpp
OptionType parseOptionType(const std::string& text)
```

This function converts CSV text into the internal enum.

It first converts the text to uppercase:

```cpp
upperText += static_cast<char>(std::toupper(...));
```

Then it checks:

```cpp
if (upperText == "CALL") {
    return CALL;
}
return PUT;
```

This means the CSV can contain `CALL`, `call`, or `Call`, and the program will
still understand it.

## isValidOption

```cpp
bool isValidOption(const Option& option)
```

This function checks whether the option parameters are mathematically valid.

It requires:

```text
spot > 0
strike > 0
rate >= 0
volatility > 0
maturity > 0
```

The function avoids nonsensical input such as a negative stock price or zero
volatility.

Invalid options are skipped when reading the CSV file.

## splitCsvLine

```cpp
std::vector<std::string> splitCsvLine(const std::string& line)
```

This function splits one CSV line at commas.

Example:

```text
AAPL Put Example,291.00,300.00,0.045,0.25,1.0,PUT
```

becomes a vector of strings:

```text
["AAPL Put Example", "291.00", "300.00", "0.045", "0.25", "1.0", "PUT"]
```

It uses:

```cpp
std::stringstream stream(line);
std::getline(stream, value, ',');
```

This is a simple parser. It is enough here because the CSV file is deliberately
kept simple and does not contain quoted commas.

## loadOptionsFromCsv

```cpp
std::vector<Option> loadOptionsFromCsv(const std::string& fileName)
```

This function reads the option data from `data/options.csv`.

Steps:

1. Open the CSV file with `std::ifstream`.
2. Skip the header row.
3. Read each remaining line.
4. Split the line into fields with `splitCsvLine`.
5. Convert numeric fields with `std::stod`.
6. Convert the option type with `parseOptionType`.
7. Validate the option with `isValidOption`.
8. Store valid options in a `std::vector<Option>`.

Important code part:

```cpp
options.push_back(option);
```

This adds each valid option to the container.

The `try/catch` block prevents the program from crashing if a CSV value cannot
be converted to a number.

## readLatestClosePrice

```cpp
bool readLatestClosePrice(const std::string& fileName, double& closePrice)
```

This function reads a small generated CSV file such as:

```text
data/AAPL_latest_price.csv
```

The file format is:

```csv
symbol,date,close
AAPL,example-date,195.0000
```

The function extracts the `close` value and stores it in the reference
parameter:

```cpp
double& closePrice
```

It returns `true` if reading worked and the close price is positive.

It returns `false` if the file is missing, malformed, or contains invalid data.

## updateMarketSpotFromApi

```cpp
bool updateMarketSpotFromApi(std::vector<Option>& options, const std::string& symbol)
```

This function implements the optional API update.

The C++ function calls `fetch_data.sh`. The script checks whether an API key is
available as an environment variable or in the local `.alphavantage_key` file.
This file is ignored by Git, so the key is not uploaded to GitHub.

If no key exists, the API update returns `false` and the program keeps using the
fallback values from `data/options.csv`.

If a key exists, it calls:

```cpp
./fetch_data.sh AAPL
```

using:

```cpp
std::system(...)
```

The shell script downloads daily stock data for the selected symbol from Alpha
Vantage and writes the latest close price to a CSV file.

Then `readLatestClosePrice` reads the latest close price.

Finally, every option whose name contains the selected symbol, for example
`"AAPL"`, receives the updated spot:

```cpp
option.spot = latestSpot;
```

The function returns `true` if at least one matching option was updated.

This design is presentation-safe:

- if the API works, fresh market data is used
- if the API fails, the program still works with `data/options.csv`

## priceOption

```cpp
double priceOption(const Option& option, int steps, ExerciseStyle style)
```

This is the core pricing function.

It prices either an American or European option, depending on `style`.

First it validates the input:

```cpp
if (!isValidOption(option) || steps <= 0) {
    return 0.0;
}
```

Then it computes the binomial model parameters.

### Time Step

```cpp
const double dt = option.maturity / steps;
```

`dt` is the length of one time step.

### Up And Down Factors

```cpp
const double up = std::exp(option.volatility * std::sqrt(dt));
const double down = 1.0 / up;
```

These are the Cox-Ross-Rubinstein up and down factors.

### Step Multiplier

```cpp
const double stepMultiplier = up / down;
```

This avoids recalculating powers for every stock price in a tree level.

Moving from one node to the next in the same level changes the stock price by:

```text
up / down = up^2
```

This is a small optimization.

### Discount Factor

```cpp
const double discount = std::exp(-option.rate * dt);
```

This discounts a future expected value back by one time step.

### Risk-Neutral Probability

```cpp
const double probability =
    (std::exp(option.rate * dt) - down) / (up - down);
```

This is the risk-neutral probability in the CRR model.

It is not a real-world probability. It is the probability used for pricing
under the risk-neutral measure.

### Memory Efficient Tree Storage

```cpp
std::vector<double> optionValues(steps + 1);
```

Instead of storing the full tree, the program stores only one level of option
values.

This reduces memory use from roughly:

```text
O(N^2)
```

to:

```text
O(N)
```

### Initialize Values At Maturity

```cpp
double stockPrice = option.spot * std::pow(down, steps);
for (int i = 0; i <= steps; ++i) {
    optionValues[i] = payoff(stockPrice, option.strike, option.type);
    stockPrice *= stepMultiplier;
}
```

At maturity the option value equals the payoff.

The loop starts at the lowest stock price and moves upward node by node.

### Backward Induction

```cpp
for (int step = steps - 1; step >= 0; --step)
```

The algorithm moves backwards from maturity to today.

At each node:

```cpp
const double continuationValue =
    discount * (probability * optionValues[i + 1] +
                (1.0 - probability) * optionValues[i]);
```

This is the discounted expected future value.

Then:

```cpp
const double exerciseValue = payoff(stockPrice, option.strike, option.type);
```

This is the value if the option is exercised immediately.

For American options:

```cpp
optionValues[i] = std::max(continuationValue, exerciseValue);
```

For European options:

```cpp
optionValues[i] = continuationValue;
```

Finally:

```cpp
return optionValues[0];
```

The first value is the option price today.

## printValuesAtStep

```cpp
void printValuesAtStep(const std::vector<double>& values, int step)
```

This helper prints all values in one tree level.

It is used for the small terminal tree preview.

It accepts a vector by constant reference, so it does not copy the data.

## printTreePreview

```cpp
void printTreePreview(const Option& option)
```

This function creates a small 3-step tree and prints it to the terminal.

Unlike the main pricing function, this function stores the full small tree:

```cpp
std::vector<std::vector<double>> stockTree;
std::vector<std::vector<double>> optionTree;
```

This is done only for visualization. A 3-step tree is tiny and easy to show.

The function prints:

- input option
- up factor
- down factor
- risk-neutral probability
- stock price tree
- American option value tree

This helps explain what the program is calculating.

## writeTreeSvg

```cpp
bool writeTreeSvg(const Option& option, const std::string& fileName)
```

This function creates a visual SVG file:

```text
binomial_tree.svg
```

It computes the same small 3-step tree as `printTreePreview`, but instead of
printing plain text, it writes SVG markup.

The SVG contains:

- lines between parent and child nodes
- circles for nodes
- stock price `S`
- option value `V`
- step labels

Important file output:

```cpp
std::ofstream svg(fileName);
```

Then the function writes SVG tags such as:

```cpp
<line ... />
<circle ... />
<text ... />
```

The function returns `true` if the SVG file could be written.

The generated SVG is not committed to Git because it is program output.

## printPriceTable

```cpp
void printPriceTable(const std::vector<Option>& options,
                     const std::vector<int>& stepTests)
```

This function prints the main pricing table.

For each option and each step count, it computes:

```cpp
americanPrice = priceOption(option, steps, AMERICAN);
europeanPrice = priceOption(option, steps, EUROPEAN);
earlyExercisePremium = americanPrice - europeanPrice;
```

The output columns are:

- option name
- type
- steps
- American price
- European price
- premium

The step tests show how the price stabilizes as the number of binomial steps
increases.

## printVolatilityAnalysis

```cpp
void printVolatilityAnalysis()
```

This function tests several volatility values:

```cpp
const std::vector<double> volatilityTests = {0.10, 0.20, 0.30, 0.40};
```

For each volatility, it prices an American put.

This demonstrates a key property of options:

```text
higher volatility usually increases option value
```

The function is useful because it shows parameter analysis without making the
project complicated.

## main

```cpp
int main()
```

This is the program entry point.

First it loads options from CSV:

```cpp
std::vector<Option> options = loadOptionsFromCsv("data/options.csv");
```

If the program is started from the build folder, it tries the alternative path:

```cpp
options = loadOptionsFromCsv("../data/options.csv");
```

If no data is found, the program exits:

```cpp
return 1;
```

Then it defines the step counts:

```cpp
const std::vector<int> stepTests = {25, 50, 100, 200};
```

Then it tries the optional API update:

```cpp
const std::string marketSymbol = "AAPL";
const bool marketDataUpdated = updateMarketSpotFromApi(options, marketSymbol);
```

After that it prints:

1. the main price table
2. the small terminal tree preview
3. the SVG tree
4. the volatility analysis

This gives a complete flow:

```text
load data -> optionally update market price -> price options -> visualize tree -> analyze volatility
```

## fetch_data.sh

This shell script is not C++, but it keeps the API logic simple.

It uses Alpha Vantage:

```bash
curl -s "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY..."
```

It writes:

```text
data/AAPL_daily.csv
data/AAPL_latest_price.csv
```

The C++ code reads only the latest close price.

This avoids complicated C++ networking code and keeps the main project focused
on pricing.

## run.sh

This script makes the project easy to run:

```bash
cmake -S . -B build
cmake --build build
./build/AmericanOptionPricer
```

Instead of typing three commands, the user can type:

```bash
./run.sh
```

## Important C++ Concepts In This Project

- `struct` for structured option data
- `enum` for clear option categories
- `std::vector` for dynamic containers
- nested `std::vector` for the small tree visualization
- references such as `const Option&`
- file input and output
- string parsing
- mathematical functions
- loops and nested loops
- formatted output
- input validation
- simple fallback handling

## C++ Features Used And Where They Appear

This section is useful for the oral presentation because one of the expected
questions is which C++ features are used in the implementation.

| C++ feature | Where it is used | Why it is used |
| --- | --- | --- |
| `struct` | `struct Option` | Stores all parameters of one option in one readable data object. |
| `enum` | `OptionType`, `ExerciseStyle` | Represents fixed categories: call/put and European/American. |
| `std::vector<Option>` | `loadOptionsFromCsv`, `main`, `printPriceTable` | Stores all option contracts loaded from CSV. This is the main data container. |
| `std::vector<int>` | `stepTests` in `main` | Stores tested binomial step counts: 25, 50, 100, 200. |
| `std::vector<double>` | `optionValues` in `priceOption` | Stores one level of option values during backward induction. |
| `std::vector<double>` | `volatilityTests` in `printVolatilityAnalysis` | Stores volatility values for parameter analysis. |
| `std::vector<std::vector<double>>` | `printTreePreview`, `writeTreeSvg` | Stores a small full 3-step tree for visualization. |
| `std::string` | option names, file names, CSV fields | Handles text values and file paths. |
| `std::ifstream` | `loadOptionsFromCsv`, `readLatestClosePrice` | Reads CSV input files. |
| `std::ofstream` | `writeTreeSvg` | Writes the SVG visualization file. |
| `std::stringstream` | `splitCsvLine` | Splits a CSV line into fields. |
| `std::getline` | CSV parsing | Reads lines from files and fields from CSV rows. |
| `std::stod` | `loadOptionsFromCsv`, `readLatestClosePrice` | Converts CSV strings to `double`. |
| `std::max` | `payoff`, `priceOption`, tree preview functions | Computes payoff and early exercise maximum. |
| `<cmath>` functions | `std::exp`, `std::sqrt`, `std::pow` | Implements the mathematical binomial model. |
| References | `const Option&`, `std::vector<Option>&` | Avoids unnecessary copies and allows API update of existing options. |
| `try/catch` | CSV conversion | Prevents crashes on invalid CSV rows. |
| `std::system` | `updateMarketSpotFromApi` | Calls the simple market-data shell script. |
| CMake | `CMakeLists.txt` | Builds the C++ executable in a standard way. |

## C++ Features Not Used

Some modern C++ features were not used because they would make the project more
complex without improving the core pricing demonstration.

| Feature | Why it is not used |
| --- | --- |
| Threads | The computation is small and fast. Parallel execution would add complexity but little benefit. |
| `std::optional` | Fallbacks are represented with `bool` return values, which is simpler for this project. |
| `std::variant` | The option data has a fixed structure, so a variant type is not necessary. |
| Templates | The model uses one numeric type, `double`; templates would be unnecessary abstraction. |
| Classes with inheritance | A simple `struct Option` and functions are easier to explain for this mathematical project. |
| External JSON library | API data is converted to simple CSV by `fetch_data.sh`, so C++ does not need JSON parsing. |

If asked why these features are not used, the answer is:

```text
I focused on a clear mathematical implementation. I used containers, file IO,
algorithms, validation and formatted output, but avoided features that would
make the code harder to explain without adding much value for this project.
```

## What To Emphasize In The Presentation

The most important code part is `priceOption`.

The most important mathematical idea is:

```text
American value = max(continuation value, exercise value)
```

The most important C++ container is:

```cpp
std::vector
```

The most useful demonstration is:

```bash
./run.sh
```

Then show:

- price table
- premium column
- terminal tree preview
- `binomial_tree.svg`

## Limitations

The model is intentionally simplified.

- no dividends
- constant volatility
- constant interest rate
- no transaction costs
- no calibration to market option prices
- API updates only the stock spot price

These limitations are acceptable for a course project because the main goal is
to demonstrate the binomial model and C++ implementation.
