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

// define new data type CALL or PUT for option type
enum OptionType {
    CALL,
    PUT
};

enum ExerciseStyle {
    EUROPEAN, // exercised at end
    AMERICAN // exercised at any time
};

struct Option {
    std::string name; // eg AAPL Put Example
    double spot;  // S_0: current price
    double strike; // K: strike price
    double rate;  // r: risk-free interest rate eg 0.045 = 4.5% per year   
    double volatility; // sigma - how volatile the stock is, eg 0.2 = 20% per year
    double maturity; // T in years - when the option expires, eg 0.5 = 6 months
    OptionType type;
};

double payoff(double stockPrice, double strike, OptionType type) { // value right now
    if (type == CALL) {
        return std::max(stockPrice - strike, 0.0);
    }
    return std::max(strike - stockPrice, 0.0);
}

std::string optionTypeName(OptionType type) { // just for output to transform enum to strin
    if (type == CALL) {
        return "Call";
    }
    return "Put";
}

OptionType parseOptionType(const std::string& text) { //convert to uppercase and compare to "CALL"/"PUT"
    std::string upperText;
    for (char c : text) {
        upperText += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    if (upperText == "CALL") {
        return CALL;
    }
    return PUT;
}

bool isValidOption(const Option& option) { //true of false for input
    return option.spot > 0.0 &&
           option.strike > 0.0 &&
           option.rate >= 0.0 &&
           option.volatility > 0.0 &&
           option.maturity > 0.0;
}

// Split a CSV line into individual values eg AAPL Put Example,291.00,300.00,0.045,0.25,1.0,PUT
std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> values;
    std::stringstream stream(line); //readable input stream from the line
    std::string value;

    while (std::getline(stream, value, ',')) { // read until comma
        values.push_back(value);
    }

    return values;
}

std::vector<Option> loadOptionsFromCsv(const std::string& fileName) {
    std::ifstream file(fileName); // read in "data/options.csv"
    std::vector<Option> options;

    if (!file.is_open()) {
        return options;
    }

    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) { //read each line of the file 
        const std::vector<std::string> values = splitCsvLine(line); //we get 7 values for each option

        if (values.size() != 7) {
            continue;
        }

        try {
            Option option = {
                values[0], //eg AAPL Put Example
                std::stod(values[1]), // do string to value eg 291.00
                std::stod(values[2]), // eg 300.00
                std::stod(values[3]), // eg 0.045
                std::stod(values[4]), // eg 0.25
                std::stod(values[5]), // eg 1.0
                parseOptionType(values[6]) // eg PUT
            };

            if (isValidOption(option)) { // call function that ecvery value is bigger than 0
                options.push_back(option);
            }
        } catch (...) {
            continue;
        }
    }

    return options;
}

bool readLatestClosePrice(const std::string& fileName, double& closePrice) {
    std::ifstream file(fileName); //open file

    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line); // skip header

    if (!std::getline(file, line)) {
        return false;
    }

    const std::vector<std::string> values = splitCsvLine(line); //again get the values

    if (values.size() != 3) {
        return false;
    }

    try {
        closePrice = std::stod(values[2]); //best case wie geht a value
    } catch (...) {
        return false;
    }

    return closePrice > 0.0;
}

bool updateMarketSpotFromApi(std::vector<Option>& options, const std::string& symbol) { //if we change options we change it outside as well 
    //try both paths and can execute the script in terminal
    int result = std::system(("./fetch_data.sh " + symbol + " > /dev/null 2>&1").c_str()); //start fetch data script
    if (result != 0) {
        result = std::system(("../fetch_data.sh " + symbol + " > /dev/null 2>&1").c_str());
    }

    if (result != 0) {
        return false;
    }

    const std::string latestPriceFile = "data/" + symbol + "_latest_price.csv";
    const std::string parentLatestPriceFile = "../data/" + symbol + "_latest_price.csv";
    double latestSpot = 0.0;

    if (!readLatestClosePrice(latestPriceFile, latestSpot) &&
        !readLatestClosePrice(parentLatestPriceFile, latestSpot)) {
        return false;
    }

    bool updated = false;

    for (Option& option : options) {
        // if symbol is in option we can update the spot price
        if (option.name.find(symbol) != std::string::npos) { 
            option.spot = latestSpot;
            updated = true;
        }
    }

    return updated;
}

double priceOption(const Option& option, int steps, ExerciseStyle style) {
    if (!isValidOption(option) || steps <= 0) {
        return 0.0;
    }

    const double dt = option.maturity / steps; // dt = T/N
    const double up = std::exp(option.volatility * std::sqrt(dt)); //u = e^(sigma * sqrt(dt))
    const double down = 1.0 / up; // d = 1 / u
    const double stepMultiplier = up / down; // up / down = up^2
    /* step 3 possible stock prices at end
    S * d^3
    S * d^2 * u
    S * d * u^2
    S * u^3
    */
    const double discount = std::exp(-option.rate * dt); // discount = e^(-r * dt) because the value in the future is worth less than the value now
    const double probability = (std::exp(option.rate * dt) - down) / (up - down); //p = (e^(r dt) - d) / (u - d)

    std::vector<double> optionValues(steps + 1); // start + steps


    double stockPrice = option.spot * std::pow(down, steps); // S0 * d^N
    //go through all END nodes and calculate the payoff for each node
    for (int i = 0; i <= steps; ++i) {
        optionValues[i] = payoff(stockPrice, option.strike, option.type);
        stockPrice *= stepMultiplier; // lowest S0*d^N -> (step) S0 * d^(N-1) * u
    }

    // Backward induction through the binomial tree
    for (int step = steps - 1; step >= 0; --step) {
        // step 2 = S0 * d^2 -> step 1 = S0 * d^1 -> step 0 = S0 * d^0
        stockPrice = option.spot * std::pow(down, step); 

        for (int i = 0; i <= step; ++i) {
            // continuation = discount * (p * upValue + (1-p) * downValue) value for holding the option
            const double continuationValue =
                discount * (probability * optionValues[i + 1] + (1.0 - probability) * optionValues[i]);
            // exercise = payoff(stockPrice, strike, type) value for exercising the option
            const double exerciseValue = payoff(stockPrice, option.strike, option.type);

            if (style == AMERICAN) {
                // take the better value
                optionValues[i] = std::max(continuationValue, exerciseValue);
            } else {
                optionValues[i] = continuationValue;
            }

            stockPrice *= stepMultiplier;
        }
    }

    return optionValues[0]; //todays value of the option at step 0
}

void printValuesAtStep(const std::vector<double>& values, int step) {
    // print values at specific tree step
    std::cout << "Step " << step << ": ";

    for (int i = 0; i <= step; ++i) {
        std::cout << std::setw(10) << values[i] << " ";
    }

    std::cout << "\n";
}

void printTreePreview(const Option& option) {
    // more steps are not useful to print 
    const int steps = 3;
    const double dt = option.maturity / steps;
    const double up = std::exp(option.volatility * std::sqrt(dt));
    const double down = 1.0 / up;
    const double discount = std::exp(-option.rate * dt);
    const double probability = (std::exp(option.rate * dt) - down) / (up - down);
    /*
    dt = T / N
    u = e^(sigma sqrt(dt))
    d = 1 / u
    discount = e^(-r dt)
    p = (e^(r dt) - d) / (u - d)
    same as the function before
    */
    std::vector<std::vector<double>> stockTree(steps + 1);
    std::vector<std::vector<double>> optionTree(steps + 1);


    // same as pricer function
    for (int step = 0; step <= steps; ++step) {
        stockTree[step].resize(step + 1);
        optionTree[step].resize(step + 1);

        for (int i = 0; i <= step; ++i) {
            stockTree[step][i] = option.spot * std::pow(up, i) * std::pow(down, step - i);
        }
    }
    
    for (int i = 0; i <= steps; ++i) {
        optionTree[steps][i] = payoff(stockTree[steps][i], option.strike, option.type);
    }
    for (int step = steps - 1; step >= 0; --step) {
        for (int i = 0; i <= step; ++i) {
            const double continuationValue =
                discount * (probability * optionTree[step + 1][i + 1] +
                            (1.0 - probability) * optionTree[step + 1][i]);
            const double exerciseValue = payoff(stockTree[step][i], option.strike, option.type);

            optionTree[step][i] = std::max(continuationValue, exerciseValue);
        }
    }

    //terminal output
    std::cout << "Small 3-Step Binomial Tree Preview\n\n";
    std::cout << "Example option: " << option.name << "\n";
    std::cout << "Spot S0:        " << option.spot << "\n";
    std::cout << "Strike K:       " << option.strike << "\n";
    std::cout << "Up factor u:    " << up << "\n";
    std::cout << "Down factor d:  " << down << "\n";
    std::cout << "Risk-neutral p: " << probability << "\n\n";

    std::cout << "Stock price tree:\n";
    for (int step = 0; step <= steps; ++step) {
        printValuesAtStep(stockTree[step], step);
    }

    std::cout << "\nAmerican option value tree:\n";
    for (int step = 0; step <= steps; ++step) {
        printValuesAtStep(optionTree[step], step);
    }

    std::cout << "\nThe value at Step 0 is the option price in this small example.\n\n";
}

bool writeTreeSvg(const Option& option, const std::string& fileName) {
    const int steps = 3;
    const double dt = option.maturity / steps;
    const double up = std::exp(option.volatility * std::sqrt(dt));
    const double down = 1.0 / up;
    const double discount = std::exp(-option.rate * dt);
    const double probability = (std::exp(option.rate * dt) - down) / (up - down);

    std::vector<std::vector<double>> stockTree(steps + 1);
    std::vector<std::vector<double>> optionTree(steps + 1);

    for (int step = 0; step <= steps; ++step) {
        stockTree[step].resize(step + 1);
        optionTree[step].resize(step + 1);

        for (int i = 0; i <= step; ++i) {
            stockTree[step][i] = option.spot * std::pow(up, i) * std::pow(down, step - i);
        }
    }

    for (int i = 0; i <= steps; ++i) {
        optionTree[steps][i] = payoff(stockTree[steps][i], option.strike, option.type);
    }

    for (int step = steps - 1; step >= 0; --step) {
        for (int i = 0; i <= step; ++i) {
            const double continuationValue =
                discount * (probability * optionTree[step + 1][i + 1] +
                            (1.0 - probability) * optionTree[step + 1][i]);
            const double exerciseValue = payoff(stockTree[step][i], option.strike, option.type);

            optionTree[step][i] = std::max(continuationValue, exerciseValue);
        }
    }


    std::ofstream svg(fileName); //write in svg file

    if (!svg.is_open()) {
        return false;
    }

    //setup picture size and layout
    const int width = 900;
    const int height = 520;
    const int startX = 100;
    const int stepX = 230;
    const int centerY = 270;
    const int stepY = 92;

    svg << std::fixed << std::setprecision(2); // eg 291.00
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
        << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " " << height << "\">\n";
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"#f8fafc\"/>\n";
    svg << "<text x=\"40\" y=\"45\" font-family=\"Arial\" font-size=\"26\" font-weight=\"700\" fill=\"#111827\">"
        << "3-Step Binomial Tree: " << option.name << "</text>\n";
    svg << "<text x=\"40\" y=\"78\" font-family=\"Arial\" font-size=\"15\" fill=\"#475569\">"
        << "Each node shows stock price S and American option value V.</text>\n";


    // draw tree lines
    for (int step = 0; step < steps; ++step) {
        for (int i = 0; i <= step; ++i) {
            const int x1 = startX + step * stepX;
            const int y1 = centerY + static_cast<int>((i - step / 2.0) * stepY);
            const int x2 = startX + (step + 1) * stepX;
            const int yDown = centerY + static_cast<int>((i - (step + 1) / 2.0) * stepY);
            const int yUp = centerY + static_cast<int>(((i + 1) - (step + 1) / 2.0) * stepY);

            svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2
                << "\" y2=\"" << yDown << "\" stroke=\"#94a3b8\" stroke-width=\"2\"/>\n";
            svg << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2
                << "\" y2=\"" << yUp << "\" stroke=\"#94a3b8\" stroke-width=\"2\"/>\n";
        }
    }

    // draw nodes and values
    for (int step = 0; step <= steps; ++step) {
        for (int i = 0; i <= step; ++i) {
            const int x = startX + step * stepX;
            const int y = centerY + static_cast<int>((i - step / 2.0) * stepY);

            svg << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"43\" fill=\"#ffffff\""
                << " stroke=\"#2563eb\" stroke-width=\"3\"/>\n";
            svg << "<text x=\"" << x << "\" y=\"" << y - 8
                << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"13\" fill=\"#111827\">"
                << "S=" << stockTree[step][i] << "</text>\n";
            svg << "<text x=\"" << x << "\" y=\"" << y + 13
                << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"13\" font-weight=\"700\" fill=\"#047857\">"
                << "V=" << optionTree[step][i] << "</text>\n";
        }
    }


    // draw step labels
    for (int step = 0; step <= steps; ++step) {
        const int x = startX + step * stepX;
        svg << "<text x=\"" << x << "\" y=\"485\" text-anchor=\"middle\" font-family=\"Arial\""
            << " font-size=\"14\" fill=\"#475569\">Step " << step << "</text>\n";
    }

    svg << "</svg>\n";
    return true;
}

void printPriceTable(const std::vector<Option>& options, const std::vector<int>& stepTests) {
    // print the table
    std::cout << "American vs European Option Prices\n\n";
    std::cout << std::left
              << std::setw(22) << "Option"
              << std::setw(8) << "Type"
              << std::setw(8) << "Steps"
              << std::setw(12) << "American"
              << std::setw(12) << "European"
              << std::setw(12) << "Premium"
              << "\n";
    std::cout << std::string(74, '-') << "\n";

    //go through options.csv 
    for (const Option& option : options) {
        if (!isValidOption(option)) {
            std::cout << option.name << " has invalid input data and is skipped.\n\n";
            continue;
        }

        for (int steps : stepTests) {
            //calculate the whole option
            const double americanPrice = priceOption(option, steps, AMERICAN);
            const double europeanPrice = priceOption(option, steps, EUROPEAN);
            //if premium is = 0 then it is not worth to exercise the option early
            const double earlyExercisePremium = americanPrice - europeanPrice;

            // print the values in a table
            std::cout << std::left
                      << std::setw(22) << option.name
                      << std::setw(8) << optionTypeName(option.type)
                      << std::setw(8) << steps
                      << std::setw(12) << americanPrice
                      << std::setw(12) << europeanPrice
                      << std::setw(12) << earlyExercisePremium
                      << "\n";
        }
        std::cout << "\n";
    }
}

void printVolatilityAnalysis() {
    Option option = {"Volatility Test Put", 100.0, 100.0, 0.05, 0.20, 1.0, PUT};
    const std::vector<double> volatilityTests = {0.10, 0.20, 0.30, 0.40};
    const int steps = 500;

    std::cout << "Volatility Analysis for an American Put\n\n";
    std::cout << std::left
              << std::setw(14) << "Volatility"
              << std::setw(12) << "Price"
              << "\n";
    std::cout << std::string(26, '-') << "\n";

    for (double volatility : volatilityTests) {
        option.volatility = volatility; //change the volatility for each test
        //calculate the price for each volatility
        const double price = priceOption(option, steps, AMERICAN);

        std::cout << std::left
                  << std::setw(14) << volatility
                  << std::setw(12) << price
                  << "\n";
    }
}


int main() {
    std::vector<Option> options = loadOptionsFromCsv("data/options.csv");

    if (options.empty()) {
        options = loadOptionsFromCsv("../data/options.csv");
    }

    if (options.empty()) {
        std::cout << "No option data found. Please check data/options.csv.\n";
        return 1;
    }

    const std::vector<int> stepTests = {25, 50, 100, 200};
    const std::string marketSymbol = "AAPL";
    //want to get the latest price from Alpha Vantage
    const bool marketDataUpdated = updateMarketSpotFromApi(options, marketSymbol);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "American Option Pricing Engine\n\n";

    if (marketDataUpdated) {
        std::cout << "Market data: " << marketSymbol << " spot price updated from Alpha Vantage.\n\n";
    } else {
        std::cout << "Market data: using fallback values from data/options.csv.\n\n";
    }

    printPriceTable(options, stepTests);
    printTreePreview(options[0]);

    if (writeTreeSvg(options[0], "binomial_tree.svg")) {
        std::cout << "Visual tree saved to binomial_tree.svg\n\n";
    }

    printVolatilityAnalysis();

    return 0;
}
