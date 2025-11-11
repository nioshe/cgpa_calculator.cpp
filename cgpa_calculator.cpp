#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace std::chrono;

enum class Difficulty {
    EASY = 1,
    MEDIUM = 2,
    HARD = 3
};

struct Metrics {
    double totalGuessTime{0.0};
    size_t guessCount{0};
    size_t fileOperations{0};
    double totalFileIOTime{0.0};
    size_t totalMemoryAllocated{0};
    size_t peakMemoryUsage{0};
    size_t scrambleCount{0};
};

struct LeaderboardEntry {
    int rank{0};
    string name;
    int score{0};
    int games{0};
    int attempts{0};
    double averageTime{0.0};
    double accuracy{0.0};
    double averageGuessTime{0.0};
    Difficulty difficulty{Difficulty::EASY};
};

class WordScrambleGame {
public:
    WordScrambleGame() {
        initializeDefaultWords();
        updateMemoryUsage();
    }

    static bool caseInsensitiveCompare(const string &lhs, const string &rhs) {
        return toLowerCase(lhs) == toLowerCase(rhs);
    }

    static string toLowerCase(const string &value) {
        string lowered = value;
        transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
            return static_cast<char>(::tolower(ch));
        });
        return lowered;
    }

    bool isValidWord(const string &word) const {
        if (word.empty()) {
            return false;
        }
        if (word.size() < 2 || word.size() > 20) {
            return false;
        }
        static const regex pattern("^[A-Za-z]+$");
        return regex_match(word, pattern);
    }

    bool addWord(const string &word) {
        string trimmed = trim(word);
        if (!isValidWord(trimmed)) {
            return false;
        }
        string lowered = toLowerCase(trimmed);
        if (uniqueWords.count(lowered) != 0) {
            return false;
        }
        words.push_back(trimmed);
        uniqueWords.insert(lowered);
        updateMemoryUsage();
        return true;
    }

    const vector<string> &getWordList() const {
        return words;
    }

    bool loadWordsFromFile(const string &filename) {
        auto start = steady_clock::now();
        ifstream input(filename);
        if (!input.is_open()) {
            return false;
        }

        string line;
        while (getline(input, line)) {
            string trimmed = trim(line);
            if (trimmed.empty()) {
                continue;
            }
            addWord(trimmed);
        }

        auto end = steady_clock::now();
        metrics.fileOperations++;
        metrics.totalFileIOTime += duration_cast<milliseconds>(end - start).count();
        updateMemoryUsage();
        return true;
    }

    void setDifficulty(Difficulty level) {
        difficulty = level;
    }

    Difficulty getDifficulty() const {
        return difficulty;
    }

    string selectRandomWord() {
        if (words.empty()) {
            return "";
        }

        uniform_int_distribution<size_t> dist(0, words.size() - 1);
        currentWord = words[dist(rng)];
        revealedPositions.clear();
        lastGuessStart = steady_clock::now();
        return currentWord;
    }

    string scrambleWord(const string &word) {
        string scrambled = word;
        if (scrambled.size() > 1) {
            shuffle(scrambled.begin(), scrambled.end(), rng);
        }
        metrics.scrambleCount++;
        return scrambled;
    }

    bool checkGuess(const string &guess) {
        auto now = steady_clock::now();
        if (lastGuessStart.time_since_epoch().count() != 0) {
            auto delta = duration_cast<milliseconds>(now - lastGuessStart).count();
            if (delta <= 0) {
                delta = 1;
            }
            metrics.totalGuessTime += static_cast<double>(delta);
        } else {
            metrics.totalGuessTime += 1.0;
        }
        metrics.guessCount++;
        lastGuessStart = steady_clock::now();

        totalGuesses++;
        attempts++;
        bool correct = caseInsensitiveCompare(guess, currentWord);
        if (correct) {
            correctGuesses++;
        }
        lastGuessCorrect = correct;
        return correct;
    }

    void updateScore() {
        if (currentWord.empty() || !lastGuessCorrect) {
            return;
        }
        int baseScore = static_cast<int>(currentWord.size()) * 10;
        auto it = customScores.find(static_cast<int>(currentWord.size()));
        if (it != customScores.end()) {
            baseScore = it->second;
        }
        double multiplier = getDifficultyMultiplier();
        score += static_cast<int>(std::round(baseScore * multiplier));
        updateMemoryUsage();
    }

    void resetAttempts() {
        attempts = 0;
    }

    void setPlayerName(const string &name) {
        playerName = name;
        updateMemoryUsage();
    }

    const string &getCurrentWord() const {
        return currentWord;
    }

    int getScore() const {
        return score;
    }

    void updateLeaderboard(double averageRoundTime) {
        LeaderboardEntry entry;
        entry.name = playerName.empty() ? string("Player") : playerName;
        entry.score = score;
        entry.games = ++gamesPlayed;
        entry.attempts = attempts > 0 ? attempts : static_cast<int>(totalGuesses);
        entry.averageTime = averageRoundTime;
        entry.accuracy = totalGuesses == 0 ? 0.0 : (static_cast<double>(correctGuesses) / totalGuesses) * 100.0;
        entry.averageGuessTime = metrics.guessCount == 0 ? 0.0 : metrics.totalGuessTime / static_cast<double>(metrics.guessCount);
        entry.difficulty = difficulty;
        leaderboard.push_back(entry);
        sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry &lhs, const LeaderboardEntry &rhs) {
            if (lhs.score == rhs.score) {
                return lhs.accuracy > rhs.accuracy;
            }
            return lhs.score > rhs.score;
        });
        for (size_t i = 0; i < leaderboard.size(); ++i) {
            leaderboard[i].rank = static_cast<int>(i + 1);
        }
        updateMemoryUsage();
    }

    bool saveMetricsToFile(const string &filename) {
        auto start = steady_clock::now();
        ofstream output(filename);
        if (!output.is_open()) {
            return false;
        }
        output << "Player: " << (playerName.empty() ? string("Player") : playerName) << '\n';
        output << "Score: " << score << '\n';
        output << "Accuracy: " << fixed << setprecision(1) << (totalGuesses == 0 ? 0.0 : (static_cast<double>(correctGuesses) / totalGuesses) * 100.0) << "%\n";
        output << "Guesses: " << totalGuesses << '\n';
        output << "Correct: " << correctGuesses << '\n';
        output << "Total Guess Time: " << fixed << setprecision(2) << metrics.totalGuessTime << " ms\n";
        output << "File I/O Operations: " << metrics.fileOperations << '\n';
        output << "Total File I/O Time: " << fixed << setprecision(2) << metrics.totalFileIOTime << " ms\n";
        output << "Scrambles: " << metrics.scrambleCount << '\n';
        output << "Total Memory: " << metrics.totalMemoryAllocated << " bytes\n";
        output << "Peak Memory: " << metrics.peakMemoryUsage << " bytes\n";
        output.close();
        auto end = steady_clock::now();
        metrics.fileOperations++;
        metrics.totalFileIOTime += duration_cast<milliseconds>(end - start).count();
        return true;
    }

    bool saveLeaderboardToFile(const string &filename) {
        auto start = steady_clock::now();
        ofstream output(filename);
        if (!output.is_open()) {
            return false;
        }
        output << "RANK,NAME,SCORE,GAMES,ATTEMPTS,AVG_TIME,ACCURACY,AVG_GUESS_TIME,DIFFICULTY" << '\n';
        for (const auto &entry : leaderboard) {
            output << entry.rank << ','
                   << entry.name << ','
                   << entry.score << ','
                   << entry.games << ','
                   << entry.attempts << ','
                   << fixed << setprecision(2) << entry.averageTime << ','
                   << fixed << setprecision(2) << entry.accuracy << ','
                   << fixed << setprecision(2) << entry.averageGuessTime << ','
                   << static_cast<int>(entry.difficulty) << '\n';
        }
        output.close();
        auto end = steady_clock::now();
        metrics.fileOperations++;
        metrics.totalFileIOTime += duration_cast<milliseconds>(end - start).count();
        return true;
    }

    bool loadLeaderboardFromFile(const string &filename) {
        auto start = steady_clock::now();
        ifstream input(filename);
        if (!input.is_open()) {
            return false;
        }
        vector<LeaderboardEntry> loaded;
        string line;
        bool headerSkipped = false;
        while (getline(input, line)) {
            if (!headerSkipped) {
                headerSkipped = true;
                if (line.find("RANK") != string::npos) {
                    continue;
                }
            }
            if (trim(line).empty()) {
                continue;
            }
            vector<string> parts = split(line, ',');
            if (parts.size() != 9) {
                continue;
            }
            LeaderboardEntry entry;
            try {
                entry.rank = stoi(parts[0]);
                entry.name = parts[1];
                entry.score = stoi(parts[2]);
                entry.games = stoi(parts[3]);
                entry.attempts = stoi(parts[4]);
                entry.averageTime = stod(parts[5]);
                entry.accuracy = stod(parts[6]);
                entry.averageGuessTime = stod(parts[7]);
                int diff = stoi(parts[8]);
                if (diff == 1) {
                    entry.difficulty = Difficulty::EASY;
                } else if (diff == 2) {
                    entry.difficulty = Difficulty::MEDIUM;
                } else if (diff == 3) {
                    entry.difficulty = Difficulty::HARD;
                } else {
                    entry.difficulty = Difficulty::EASY;
                }
            } catch (const exception &) {
                continue;
            }
            loaded.push_back(entry);
        }
        leaderboard = std::move(loaded);
        sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry &lhs, const LeaderboardEntry &rhs) {
            if (lhs.score == rhs.score) {
                return lhs.accuracy > rhs.accuracy;
            }
            return lhs.score > rhs.score;
        });
        for (size_t i = 0; i < leaderboard.size(); ++i) {
            leaderboard[i].rank = static_cast<int>(i + 1);
        }
        auto end = steady_clock::now();
        metrics.fileOperations++;
        metrics.totalFileIOTime += duration_cast<milliseconds>(end - start).count();
        updateMemoryUsage();
        return true;
    }

    void displayLeaderboard() const {
        if (leaderboard.empty()) {
            cout << "No leaderboard data available." << endl;
            return;
        }
        cout << left << setw(5) << "Rank" << setw(15) << "Name" << setw(10) << "Score"
             << setw(10) << "Games" << setw(12) << "Attempts" << setw(12) << "Avg Time"
             << setw(12) << "Accuracy" << setw(15) << "Avg Guess" << setw(12) << "Difficulty" << endl;
        for (const auto &entry : leaderboard) {
            cout << left << setw(5) << entry.rank
                 << setw(15) << entry.name
                 << setw(10) << entry.score
                 << setw(10) << entry.games
                 << setw(12) << entry.attempts
                 << setw(12) << fixed << setprecision(1) << entry.averageTime
                 << setw(12) << fixed << setprecision(1) << entry.accuracy << "%"
                 << setw(15) << fixed << setprecision(2) << entry.averageGuessTime
                 << setw(12) << difficultyToString(entry.difficulty) << endl;
        }
    }

    void showHint(int level) {
        if (currentWord.empty()) {
            cout << "No word selected." << endl;
            return;
        }
        if (revealedPositions.size() >= currentWord.size()) {
            cout << "No more hints available." << endl;
            return;
        }
        switch (level) {
        case 1:
            cout << "Starts with: " << currentWord.front() << endl;
            revealedPositions.insert(0);
            break;
        case 2:
            cout << "Starts with " << currentWord.front() << " ... ends with " << currentWord.back() << endl;
            revealedPositions.insert(0);
            revealedPositions.insert(currentWord.size() - 1);
            break;
        case 3:
        default: {
            size_t position = nextUnrevealedPosition();
            if (position >= currentWord.size()) {
                cout << "No more hints available." << endl;
                return;
            }
            revealedPositions.insert(position);
            cout << "Letter at position " << (position + 1) << " is '" << currentWord[position] << "'" << endl;
            break;
        }
        }
    }

    void customizeScoring(int wordLength, int reward) {
        if (wordLength <= 0 || reward <= 0) {
            return;
        }
        customScores[wordLength] = reward;
    }

    Metrics getMetrics() const {
        return metrics;
    }

private:
    vector<string> words;
    unordered_set<string> uniqueWords;
    vector<LeaderboardEntry> leaderboard;
    unordered_map<int, int> customScores;
    string currentWord;
    string playerName;
    bool lastGuessCorrect{false};
    size_t totalGuesses{0};
    size_t correctGuesses{0};
    int attempts{0};
    int score{0};
    int gamesPlayed{0};
    Difficulty difficulty{Difficulty::EASY};
    mutable Metrics metrics;
    unordered_set<size_t> revealedPositions;
    steady_clock::time_point lastGuessStart{};
    mt19937 rng{static_cast<unsigned>(steady_clock::now().time_since_epoch().count())};

    static string trim(const string &value) {
        size_t start = value.find_first_not_of(" \t\n\r");
        if (start == string::npos) {
            return "";
        }
        size_t end = value.find_last_not_of(" \t\n\r");
        return value.substr(start, end - start + 1);
    }

    static vector<string> split(const string &value, char delimiter) {
        vector<string> parts;
        string token;
        stringstream ss(value);
        while (getline(ss, token, delimiter)) {
            parts.push_back(token);
        }
        return parts;
    }

    void initializeDefaultWords() {
        static const vector<string> defaults{"puzzle", "challenge", "example", "solution"};
        for (const auto &word : defaults) {
            words.push_back(word);
            uniqueWords.insert(toLowerCase(word));
        }
    }

    void updateMemoryUsage() {
        size_t total = 0;
        total += words.size() * sizeof(string);
        for (const auto &word : words) {
            total += word.size();
        }
        total += uniqueWords.size() * sizeof(string);
        total += leaderboard.size() * sizeof(LeaderboardEntry);
        for (const auto &entry : leaderboard) {
            total += entry.name.size();
        }
        total += playerName.size();
        metrics.totalMemoryAllocated = total;
        metrics.peakMemoryUsage = max(metrics.peakMemoryUsage, total);
    }

    double getDifficultyMultiplier() const {
        switch (difficulty) {
        case Difficulty::EASY:
            return 1.0;
        case Difficulty::MEDIUM:
            return 1.5;
        case Difficulty::HARD:
            return 2.0;
        }
        return 1.0;
    }

    static string difficultyToString(Difficulty diff) {
        switch (diff) {
        case Difficulty::EASY:
            return "Easy";
        case Difficulty::MEDIUM:
            return "Medium";
        case Difficulty::HARD:
            return "Hard";
        }
        return "Easy";
    }

    size_t nextUnrevealedPosition() const {
        for (size_t i = 0; i < currentWord.size(); ++i) {
            if (revealedPositions.count(i) == 0) {
                return i;
            }
        }
        return currentWord.size();
    }
};
