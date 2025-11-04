#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <cctype>

using namespace std;

// Judge status enumeration
enum class Status {
    ACCEPTED,
    WRONG_ANSWER,
    RUNTIME_ERROR,
    TIME_LIMIT_EXCEED
};

// Convert string to Status
Status stringToStatus(const string& statusStr) {
    if (statusStr == "Accepted") return Status::ACCEPTED;
    if (statusStr == "Wrong_Answer") return Status::WRONG_ANSWER;
    if (statusStr == "Runtime_Error") return Status::RUNTIME_ERROR;
    if (statusStr == "Time_Limit_Exceed") return Status::TIME_LIMIT_EXCEED;
    return Status::WRONG_ANSWER; // default
}

// Convert Status to string
string statusToString(Status status) {
    switch (status) {
        case Status::ACCEPTED: return "Accepted";
        case Status::WRONG_ANSWER: return "Wrong_Answer";
        case Status::RUNTIME_ERROR: return "Runtime_Error";
        case Status::TIME_LIMIT_EXCEED: return "Time_Limit_Exceed";
        default: return "Unknown";
    }
}

struct Submission {
    string teamName;
    string problemName;
    Status status;
    int time;

    Submission(const string& team, const string& problem, Status stat, int t)
        : teamName(team), problemName(problem), status(stat), time(t) {}
};

struct Team {
    string name;
    int solvedCount;
    int penaltyTime;
    map<string, int> wrongSubmissions; // problem -> wrong submission count before first AC
    map<string, int> firstAcceptTime; // problem -> first AC time
    map<string, bool> isSolved; // problem -> whether solved
    map<string, int> totalSubmissions; // problem -> total submissions
    map<string, int> frozenSubmissions; // problem -> submissions after freeze

    Team() : name(""), solvedCount(0), penaltyTime(0) {}
    Team(const string& n) : name(n), solvedCount(0), penaltyTime(0) {}

    // Get penalty time for a problem
    int getProblemPenalty(const string& problem) const {
        if (!isSolved.at(problem)) return 0;
        return 20 * wrongSubmissions.at(problem) + firstAcceptTime.at(problem);
    }

    // Get maximum solve time
    int getMaxSolveTime() const {
        int maxTime = 0;
        for (const auto& [problem, solved] : isSolved) {
            if (solved) {
                maxTime = max(maxTime, firstAcceptTime.at(problem));
            }
        }
        return maxTime;
    }

    // Get solve times in descending order
    vector<int> getSolveTimes() const {
        vector<int> times;
        for (const auto& [problem, solved] : isSolved) {
            if (solved) {
                times.push_back(firstAcceptTime.at(problem));
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
};

class ICPCManagementSystem {
private:
    bool competitionStarted;
    bool isFrozen;
    int durationTime;
    int problemCount;
    vector<string> problemNames;

    map<string, Team> teams;
    vector<Submission> submissions;
    vector<string> teamOrder; // Current ranking order

    // For scroll operation
    map<string, set<string>> frozenProblems; // team -> set of frozen problems

public:
    ICPCManagementSystem() : competitionStarted(false), isFrozen(false), durationTime(0), problemCount(0) {}

    void addTeam(const string& teamName) {
        if (competitionStarted) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }

        if (teams.find(teamName) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        teams[teamName] = Team(teamName);
        teamOrder.push_back(teamName);
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problemCnt) {
        if (competitionStarted) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        durationTime = duration;
        problemCount = problemCnt;

        // Generate problem names (A, B, C, ...)
        for (int i = 0; i < problemCount; ++i) {
            problemNames.push_back(string(1, 'A' + i));
        }

        // Initialize all teams with problems
        for (auto& [name, team] : teams) {
            for (const auto& problem : problemNames) {
                team.wrongSubmissions[problem] = 0;
                team.firstAcceptTime[problem] = 0;
                team.isSolved[problem] = false;
                team.totalSubmissions[problem] = 0;
                team.frozenSubmissions[problem] = 0;
            }
        }

        competitionStarted = true;
        cout << "[Info]Competition starts.\n";
    }

    void submitProblem(const string& problemName, const string& teamName, const string& statusStr, int time) {
        Status status = stringToStatus(statusStr);
        submissions.emplace_back(teamName, problemName, status, time);

        Team& team = teams[teamName];
        team.totalSubmissions[problemName]++;

        if (isFrozen && !team.isSolved[problemName]) {
            // After freeze, count submissions but don't update solved status
            team.frozenSubmissions[problemName]++;
            if (status == Status::ACCEPTED) {
                frozenProblems[teamName].insert(problemName);
            }
        } else {
            // Before freeze or already solved problem
            if (!team.isSolved[problemName]) {
                if (status == Status::ACCEPTED) {
                    team.isSolved[problemName] = true;
                    team.firstAcceptTime[problemName] = time;
                    team.solvedCount++;
                    team.penaltyTime += team.getProblemPenalty(problemName);
                } else {
                    team.wrongSubmissions[problemName]++;
                }
            }
        }

        // No output for SUBMIT command
    }

    void flushScoreboard() {
        updateRankings();
        cout << "[Info]Flush scoreboard.\n";
        printScoreboard();
    }

    void freezeScoreboard() {
        if (isFrozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        isFrozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scrollScoreboard() {
        if (!isFrozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush the scoreboard
        updateRankings();
        printScoreboard();

        // Process scroll operation
        vector<pair<string, string>> rankingChanges;

        while (true) {
            // Find the lowest-ranked team with frozen problems
            string teamToUnfreeze = "";
            string problemToUnfreeze = "";

            for (int i = teamOrder.size() - 1; i >= 0; --i) {
                const string& teamName = teamOrder[i];
                if (!frozenProblems[teamName].empty()) {
                    teamToUnfreeze = teamName;
                    // Find the problem with smallest letter
                    problemToUnfreeze = *frozenProblems[teamName].begin();
                    for (const auto& problem : frozenProblems[teamName]) {
                        if (problem < problemToUnfreeze) {
                            problemToUnfreeze = problem;
                        }
                    }
                    break;
                }
            }

            if (teamToUnfreeze.empty()) break;

            // Unfreeze the problem
            Team& team = teams[teamToUnfreeze];
            frozenProblems[teamToUnfreeze].erase(problemToUnfreeze);

            // Check if this problem was solved during freeze
            if (team.frozenSubmissions[problemToUnfreeze] > 0) {
                // Check if any submission during freeze was AC
                // We need to find the first AC submission during freeze
                int firstACTime = -1;
                for (const auto& submission : submissions) {
                    if (submission.teamName == teamToUnfreeze &&
                        submission.problemName == problemToUnfreeze &&
                        submission.status == Status::ACCEPTED &&
                        submission.time > 0) { // time > 0 indicates after freeze
                        if (firstACTime == -1 || submission.time < firstACTime) {
                            firstACTime = submission.time;
                        }
                    }
                }

                if (firstACTime != -1) {
                    // Problem was solved during freeze
                    team.isSolved[problemToUnfreeze] = true;
                    team.firstAcceptTime[problemToUnfreeze] = firstACTime;
                    team.solvedCount++;
                    team.penaltyTime += team.getProblemPenalty(problemToUnfreeze);

                    // Update rankings and check for changes
                    vector<string> oldOrder = teamOrder;
                    updateRankings();

                    // Check if ranking changed
                    if (oldOrder != teamOrder) {
                        // Find the team that was replaced
                        string replacedTeam = "";
                        for (size_t i = 0; i < oldOrder.size(); ++i) {
                            if (oldOrder[i] == teamToUnfreeze) {
                                if (i > 0) replacedTeam = oldOrder[i-1];
                                break;
                            }
                        }

                        if (!replacedTeam.empty()) {
                            rankingChanges.emplace_back(teamToUnfreeze, replacedTeam);
                        }
                    }
                }
            }

            team.frozenSubmissions[problemToUnfreeze] = 0;
        }

        // Output ranking changes
        for (const auto& [team1, team2] : rankingChanges) {
            cout << team1 << " " << team2 << " "
                 << teams[team1].solvedCount << " "
                 << teams[team1].penaltyTime << "\n";
        }

        // Output final scoreboard
        printScoreboard();

        isFrozen = false;
        frozenProblems.clear();
    }

    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (isFrozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        // Find current ranking
        int ranking = 1;
        for (const auto& name : teamOrder) {
            if (name == teamName) {
                cout << teamName << " NOW AT RANKING " << ranking << "\n";
                return;
            }
            ranking++;
        }
    }

    void querySubmission(const string& teamName, const string& problemName, const string& statusStr) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Status targetStatus = (statusStr == "ALL") ? Status::ACCEPTED : stringToStatus(statusStr);
        bool allProblems = (problemName == "ALL");
        bool allStatuses = (statusStr == "ALL");

        // Find the last matching submission
        Submission* lastMatch = nullptr;
        for (auto it = submissions.rbegin(); it != submissions.rend(); ++it) {
            if (it->teamName == teamName &&
                (allProblems || it->problemName == problemName) &&
                (allStatuses || it->status == targetStatus)) {
                lastMatch = &(*it);
                break;
            }
        }

        if (lastMatch == nullptr) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << lastMatch->teamName << " "
                 << lastMatch->problemName << " "
                 << statusToString(lastMatch->status) << " "
                 << lastMatch->time << "\n";
        }
    }

    void endCompetition() {
        cout << "[Info]Competition ends.\n";
    }

private:
    void updateRankings() {
        // Sort teams according to ranking rules
        sort(teamOrder.begin(), teamOrder.end(), [this](const string& a, const string& b) {
            const Team& teamA = teams[a];
            const Team& teamB = teams[b];

            // 1. More solved problems ranks higher
            if (teamA.solvedCount != teamB.solvedCount) {
                return teamA.solvedCount > teamB.solvedCount;
            }

            // 2. Less penalty time ranks higher
            if (teamA.penaltyTime != teamB.penaltyTime) {
                return teamA.penaltyTime < teamB.penaltyTime;
            }

            // 3. Compare solve times in descending order
            vector<int> timesA = teamA.getSolveTimes();
            vector<int> timesB = teamB.getSolveTimes();

            size_t minSize = min(timesA.size(), timesB.size());
            for (size_t i = 0; i < minSize; ++i) {
                if (timesA[i] != timesB[i]) {
                    return timesA[i] < timesB[i];
                }
            }

            // 4. Lexicographic order of team names
            return a < b;
        });
    }

    void printScoreboard() {
        for (size_t i = 0; i < teamOrder.size(); ++i) {
            const string& teamName = teamOrder[i];
            const Team& team = teams[teamName];

            cout << teamName << " " << (i + 1) << " "
                 << team.solvedCount << " " << team.penaltyTime;

            for (const auto& problem : problemNames) {
                cout << " ";

                if (team.isSolved.at(problem)) {
                    // Problem solved
                    int wrongBefore = team.wrongSubmissions.at(problem);
                    if (wrongBefore == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << wrongBefore;
                    }
                } else if (frozenProblems[teamName].count(problem) > 0) {
                    // Problem is frozen
                    int wrongBefore = team.wrongSubmissions.at(problem);
                    int frozenCount = team.frozenSubmissions.at(problem);
                    if (wrongBefore == 0) {
                        cout << "0/" << frozenCount;
                    } else {
                        cout << "-" << wrongBefore << "/" << frozenCount;
                    }
                } else {
                    // Problem not solved and not frozen
                    int wrongCount = team.wrongSubmissions.at(problem);
                    if (wrongCount == 0) {
                        cout << ".";
                    } else {
                        cout << "-" << wrongCount;
                    }
                }
            }

            cout << "\n";
        }
    }
};

int main() {
    ICPCManagementSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            system.addTeam(teamName);
        } else if (command == "START") {
            string dummy;
            int duration, problemCount;
            iss >> dummy >> duration >> dummy >> problemCount;
            system.startCompetition(duration, problemCount);
        } else if (command == "SUBMIT") {
            string problemName, by, teamName, with, statusStr, at;
            int time;
            iss >> problemName >> by >> teamName >> with >> statusStr >> at >> time;
            system.submitProblem(problemName, teamName, statusStr, time);
        } else if (command == "FLUSH") {
            system.flushScoreboard();
        } else if (command == "FREEZE") {
            system.freezeScoreboard();
        } else if (command == "SCROLL") {
            system.scrollScoreboard();
        } else if (command == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        } else if (command == "QUERY_SUBMISSION") {
            string teamName, where, problemEquals, problemName, and_str, statusEquals, statusStr;
            iss >> teamName >> where >> problemEquals >> problemName >> and_str >> statusEquals >> statusStr;
            // Remove "PROBLEM=" and "STATUS=" prefixes if present
            if (problemName.find("PROBLEM=") == 0) {
                problemName = problemName.substr(8);
            }
            if (statusStr.find("STATUS=") == 0) {
                statusStr = statusStr.substr(7);
            }
            system.querySubmission(teamName, problemName, statusStr);
        } else if (command == "END") {
            system.endCompetition();
            break;
        }
    }

    return 0;
}