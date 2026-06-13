#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <unordered_set>
#include <ctime>
#include <map>
#include <limits>
#include <cctype>
#include <climits>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
using namespace std;

/* ===================== Helpers ===================== */
int getValidatedInt(const string& prompt, int minVal = INT_MIN, int maxVal = INT_MAX) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            if (value >= minVal && value <= maxVal) break;
            else cout << "Value must be between " << minVal << " and " << maxVal << endl;
        }
        else {
            cout << "Invalid input! Enter a number." << endl;
            cin.clear();
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return value;
}

string getValidatedString(const string& prompt, bool lettersOnly = true, int exactLen = 0) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        bool valid = true;
        if (exactLen > 0 && (int)input.length() != exactLen) {
            cout << "Input must be exactly " << exactLen << " characters long." << endl;
            continue;
        }
        for (char ch : input) {
            if (lettersOnly) { if (!isalpha(ch) && ch != ' ') { valid = false; break; } }
            else { if (!isalnum(ch)) { valid = false; break; } }
        }
        if (valid) break;
        cout << "Invalid input! " << (lettersOnly ? "Only letters and spaces allowed.\n" : "Only letters and numbers allowed.\n");
    }
    return input;
}

/* ===================== Lightweight hash ===================== */
static string simpleHash(const string& s) {
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    stringstream ss; ss << hex << h; return ss.str();
}

/* ===================== Vote ===================== */
class Vote {
public:
    string voteID, voterID, candidateID, electionID, timestamp, hash;
    Vote(string vID, string voter, string cand, string elec) :
        voteID(vID), voterID(voter), candidateID(cand), electionID(elec) {
        time_t now = time(nullptr);
        // Portable formatting
        tm localTm;
        localtime_s(&localTm, &now);
        stringstream ts;
        ts << put_time(&localTm, "%Y-%m-%d %H:%M:%S");
        timestamp = ts.str();

    }
    void computeHash() {
        hash = simpleHash(voteID + "|" + voterID + "|" + candidateID + "|" + electionID + "|" + timestamp);
    }
    bool verifyHash() const {
        return hash == simpleHash(voteID + "|" + voterID + "|" + candidateID + "|" + electionID + "|" + timestamp);
    }
    void print() const {
        cout << "Vote " << voteID << ": " << voterID << " -> " << candidateID
            << " (Election " << electionID << ") at " << timestamp << " [hash " << hash << "]" << endl;
    }
    string serializeCSV() const {
        return voteID + "," + voterID + "," + candidateID + "," + electionID + "," + timestamp + "," + hash;
    }
    static Vote deserializeCSV(const string& line) {
        vector<string> parts; string tmp; stringstream ss(line);
        while (getline(ss, tmp, ',')) parts.push_back(tmp);
        // Expect 6 fields; if malformed, return dummy and caller can skip
        Vote v(parts.size() > 0 ? parts[0] : "", parts.size() > 1 ? parts[1] : "",
            parts.size() > 2 ? parts[2] : "", parts.size() > 3 ? parts[3] : "");
        if (parts.size() > 4) v.timestamp = parts[4];
        v.computeHash();
        if (parts.size() > 5) v.hash = parts[5];
        return v;
    }
};

/* ===================== User (abstract) ===================== */
class User {
public:
    string userID, name, role, passwordHash, cnic;
    User(string id, string n, string r, string pwdHash, string cnicNum = "")
        : userID(id), name(n), role(r), passwordHash(pwdHash), cnic(cnicNum) {
    }
    bool login(const string& pwd) { return passwordHash == simpleHash(pwd); }
    virtual void displayInfo() const = 0;
};

/* ===================== Profiles ===================== */
class VoterProfile {
public:
    int age = 0;
    string gender, constituency;
    set<string> issueSet;
    void updateProfile(int a, const string& g, const string& c, const set<string>& issues) {
        age = a; gender = g; constituency = c; issueSet = issues;
    }
    void getProfileInfo() const {
        cout << "Age: " << age << ", Gender: " << gender << ", Constituency: " << constituency << endl;
    }
};

class CandidateProfile {
public:
    string party; int experienceYears = 0; set<string> supportedIssues;
    void updateProfile(const string& p, int exp, const set<string>& issues) {
        party = p; experienceYears = exp; supportedIssues = issues;
    }
    void getProfileInfo() const {
        cout << "Party: " << party << ", Experience: " << experienceYears << " years" << endl;
    }
};

/* ===================== Concrete users ===================== */
class Voter : public User {
public:
    int age; string constituency; set<string> issues; bool registered;
    Voter(string id, string n, const string& pwd, string cnicNum, int a, string cons)
        : User(id, n, "Voter", simpleHash(pwd), cnicNum), age(a), constituency(cons), registered(true) {
    }
    bool isEligible() const { return age >= 18 && registered; }
    Vote castVote(const string& candidateID, const string& electionID) {
        string vID = userID + "_v_" + electionID;
        cout << name << " casts vote..." << endl; return Vote(vID, userID, candidateID, electionID);
    }
    void displayInfo() const override {
        cout << "Voter: " << name << " (ID " << userID << "), Age: " << age << ", Constituency: " << constituency << endl;
    }
};

class Candidate : public User {
public:
    string party; int expYears; int votesReceived; set<string> supportedIssues;
    Candidate(string id, string n, const string& pwd, string pty, int exp, string cnicNum = "")
        : User(id, n, "Candidate", simpleHash(pwd), cnicNum), party(pty), expYears(exp), votesReceived(0) {
    }
    void addVote() { votesReceived++; }
    void displayInfo() const override {
        cout << "Candidate: " << name << " (Party: " << party << ", Votes: " << votesReceived << ")" << endl;
    }
};

class Admin : public User {
public:
    Admin(string id, string n, const string& pwd) : User(id, n, "Admin", simpleHash(pwd)) {}
    void displayInfo() const override { cout << "Admin: " << name << endl; }
    void showLeadingParty(const map<string, int>& partyVotes) const {
        string leader; int maxVotes = -1;
        for (auto& p : partyVotes) if (p.second > maxVotes) { maxVotes = p.second; leader = p.first; }
        cout << "Current Leading Party: " << leader << " with " << maxVotes << " votes." << endl;
    }
};

class Auditor : public User {
public:
    Auditor(string id, string n, const string& pwd) : User(id, n, "Auditor", simpleHash(pwd)) {}
    void displayInfo() const override { cout << "Auditor: " << name << endl; }
    void viewVotes(const vector<Vote>& ledger) const {
        cout << "Auditor " << name << " viewing votes:" << endl;
        for (auto& v : ledger) v.print();
    }
    void verifyLedger(const vector<Vote>& ledger) const {
        int ok = 0, bad = 0;
        for (auto& v : ledger) (v.verifyHash() ? ok : bad)++;
        cout << "Ledger verification: " << ok << " valid, " << bad << " tampered." << endl;
    }
};

/* ===================== AuthManager ===================== */
class AuthManager {
public:
    map<string, User*> users; // id -> User*
    ~AuthManager() { for (auto& kv : users) delete kv.second; }
    void registerUser(User* u) { users[u->userID] = u; }
    User* login(const string& id, const string& pwd) {
        auto it = users.find(id);
        if (it != users.end() && it->second->login(pwd)) return it->second;
        return nullptr;
    }
};

/* ===================== FileStorage ===================== */
class FileStorage {
public:
    // Save all candidates (already in your code)
    void saveCandidates(const vector<Candidate>& candidates) {
        ofstream fout("candidates.csv");
        for (auto& c : candidates) {
            fout << c.userID << "," << c.name << "," << c.party << ","
                << c.expYears << "," << c.votesReceived << "\n";
        }
    }

    vector<Candidate> loadCandidates() {
        vector<Candidate> result;
        ifstream fin("candidates.csv");
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, name, party;
            int exp, votes;
            getline(ss, id, ',');
            getline(ss, name, ',');
            getline(ss, party, ',');
            ss >> exp; ss.ignore(); ss >> votes;
            Candidate c(id, name, "123", party, exp);
            c.votesReceived = votes;
            result.push_back(c);
        }
        return result;
    }

    // 🔹 NEW: Save a single vote to file
    void saveVote(const Vote& v) {
        ofstream fout("votes.csv", ios::app); // append mode
        fout << v.voteID << "," << v.voterID << "," << v.candidateID << ","
            << v.electionID << "," << v.timestamp << "," << v.hash << "\n";
    }

    // 🔹 NEW: Load all votes from file
    vector<Vote> loadVotes() {
        vector<Vote> result;
        ifstream fin("votes.csv");
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string vID, voterID, candID, electionID, timestamp, hash;
            getline(ss, vID, ',');
            getline(ss, voterID, ',');
            getline(ss, candID, ',');
            getline(ss, electionID, ',');
            getline(ss, timestamp, ',');
            getline(ss, hash, ',');
            Vote v(vID, voterID, candID, electionID);
            v.timestamp = timestamp;
            v.hash = hash;
            result.push_back(v);
        }
        return result;
    }
};

/* ===================== Election (abstract) ===================== */
class Election {
public:
    string electionID, name; bool isActive = true;
    set<string> eligibleVoterSet, candidateSet;
    virtual ~Election() = default;
    virtual void addCandidate(const string& candID) { candidateSet.insert(candID); }
    virtual void addVoterEligibility(const string& voterID) { eligibleVoterSet.insert(voterID); }
    virtual void closeElection() { isActive = false; }
};

/* ===================== ElectionManager ===================== */
class ElectionManager {
public:
    map<string, Election> elections; // concrete basic Election
    map<string, map<string, string>> voteRelationTable; // electionID -> voterID -> candidateID
    map<string, map<string, int>> voteCountTable;       // electionID -> candidateID -> count

    void createElection(const string& id, const string& name) {
        Election e; e.electionID = id; e.name = name; e.isActive = true;
        elections[id] = e;
    }
    bool hasVoted(const string& voterID, const string& electionID) {
        return voteRelationTable[electionID].count(voterID) > 0;
    }
    void registerCandidateToElection(const string& electionID, const string& candID) {
        elections[electionID].addCandidate(candID);
    }
    void registerVoterToElection(const string& electionID, const string& voterID) {
        elections[electionID].addVoterEligibility(voterID);
    }
    bool castVote(const string& voterID, const string& candidateID, const string& electionID) {
        if (!elections.count(electionID) || !elections[electionID].isActive) return false;
        voteRelationTable[electionID][voterID] = candidateID;
        voteCountTable[electionID][candidateID]++;
        return true;
    }
    void tally(const string& electionID) {
        cout << "Tally for election " << electionID << ":\n";
        for (auto& kv : voteCountTable[electionID]) {
            cout << "Candidate " << kv.first << " -> " << kv.second << " votes\n";
        }
    }
};

/* ===================== EligibilityChecker ===================== */
class EligibilityChecker {
public:
    bool checkAge(const Voter& v) { return v.age >= 18; }
    bool checkRegistration(const Voter& v) { return v.registered; }
    bool checkVotedBefore(const Voter& v, const string& electionID, ElectionManager& em) {
        return !em.hasVoted(v.userID, electionID);
    }
    bool validate(const Voter& v, const string& electionID, ElectionManager& em) {
        return checkAge(v) && checkRegistration(v) && checkVotedBefore(v, electionID, em);
    }
};

/* ===================== RecommendationEngine ===================== */
class RecommendationEngine {
public:
    string activeStrategy = "issueMatch";
    void setStrategy(const string& s) { activeStrategy = s; }
    vector<string> recommendFor(const Voter& v, const vector<Candidate>& candidates) {
        // Simple issueMatch: recommend candidates sharing at least one issue
        vector<string> recs;
        for (const auto& c : candidates) {
            vector<string> common;
            set<string> vi = v.issues, ci = c.supportedIssues;
            for (const auto& issue : vi) if (ci.count(issue)) common.push_back(issue);
            if (!common.empty()) recs.push_back(c.userID); // return IDs; map to names in UI
        }
        return recs;
    }
};
int main() {
    vector<Candidate> candidates;
    map<string, int> partyVotes;
    vector<Vote> ledger;
    unordered_set<string> usedCNIC;


    FileStorage storage;
    AuthManager auth;
    ElectionManager em;
    EligibilityChecker rules;
    RecommendationEngine recommender;

    // 🔹 Load candidates from file at startup
    candidates = storage.loadCandidates();

    // If file was empty, pre-register defaults
    if (candidates.empty()) {
        candidates.push_back(Candidate("c1", "Nawaz Sharif", "123", "PML(N)", 3));
        candidates.push_back(Candidate("c2", "Bilawal Bhutto", "123", "PPP", 2));
        candidates.push_back(Candidate("c3", "Imran Khan", "123", "PTI", 4));
    }

    // Initialize partyVotes from loaded candidates
    for (auto& c : candidates) {
        partyVotes[c.party] = c.votesReceived;
        auth.registerUser(new Candidate(c)); // register in AuthManager
    }

    // Pre-create default election
    em.createElection("E2025", "General Election 2025");
    for (auto& c : candidates) {
        em.registerCandidateToElection("E2025", c.userID);
    }

    // Register system users
    Admin admin("adm1", "AdminOne", "123");
    Auditor auditor("aud1", "MrAudit", "123");
    auth.registerUser(new Admin(admin));
    auth.registerUser(new Auditor(auditor));

    // 🔹 CLI Loop
    while (true) {
        int roleChoice = getValidatedInt(
            "\nSelect Role:\n1) Voter\n2) Candidate\n3) Admin\n4) Auditor\n5) Exit\nChoice: ", 1, 5);
        if (roleChoice == 5) break;

        if (roleChoice == 1) {
            string name = getValidatedString("Enter Name: ");
            string pwd; cout << "Enter Password: "; getline(cin, pwd);
            string cnic = getValidatedString("Enter CNIC (13 digits): ", false, 13);
            int age = getValidatedInt("Enter Age: ", 0, 150);
            string cons = getValidatedString("Enter Constituency: ");

            Voter v1("v_" + cnic, name, pwd, cnic, age, cons);

            // Collect issues
            cout << "Enter issues separated by spaces (e.g., education health jobs): ";
            string issuesLine; getline(cin, issuesLine);
            stringstream iss(issuesLine); string tok;
            while (iss >> tok) v1.issues.insert(tok);

            auth.registerUser(new Voter(v1));
            em.registerVoterToElection("E2025", v1.userID);

            if (!rules.validate(v1, "E2025", em)) {
                cout << "You are not eligible to vote.\n";
                continue;
            }

            // Recommendations
            auto recs = recommender.recommendFor(v1, candidates);
            if (!recs.empty()) {
                cout << "Recommended candidates: ";
                for (auto& id : recs) {
                    auto it = find_if(candidates.begin(), candidates.end(),
                        [&](const Candidate& c) { return c.userID == id; });
                    if (it != candidates.end()) cout << it->name << " ";
                }
                cout << "\n";
            }

            cout << "Select Candidate to vote:\n";
            for (int i = 0; i < (int)candidates.size(); i++)
                cout << i + 1 << ") " << candidates[i].name << " (" << candidates[i].party << ")\n";

            int voteChoice = getValidatedInt("Choice: ", 1, (int)candidates.size());
            string chosenID = candidates[voteChoice - 1].userID;

            if (!em.castVote(v1.userID, chosenID, "E2025")) {
                cout << "Failed to cast vote.\n";
                continue;
            }

            Vote vote = v1.castVote(chosenID, "E2025");
            ledger.push_back(vote);
            storage.saveVote(vote);
            candidates[voteChoice - 1].addVote();
            partyVotes[candidates[voteChoice - 1].party]++;
            usedCNIC.insert(cnic);

            cout << "Your vote has been recorded.\n";
        }

        else if (roleChoice == 2) {
            string id = getValidatedString("Enter Candidate ID: ", false);
            string pwd; cout << "Enter Password: "; getline(cin, pwd);
            User* u = auth.login(id, pwd);
            if (!u || u->role != "Candidate") { cout << "Invalid Candidate ID or Password!\n"; continue; }
            auto it = find_if(candidates.begin(), candidates.end(), [&](const Candidate& c) { return c.userID == id; });
            if (it == candidates.end()) { cout << "Candidate not found.\n"; continue; }

            int candOpt = 0;
            while (candOpt != 3) {
                candOpt = getValidatedInt("\nCandidate Menu:\n1) View Profile\n2) View Votes\n3) Logout\nChoice: ", 1, 3);
                if (candOpt == 1) it->displayInfo();
                else if (candOpt == 2) cout << it->votesReceived << " votes received.\n";
            }
        }

        else if (roleChoice == 3) {
            string id = getValidatedString("Enter Admin ID: ", false);
            string pwd; cout << "Enter Admin Password: "; getline(cin, pwd);
            User* u = auth.login(id, pwd);
            if (!u || u->role != "Admin") { cout << "Wrong credentials!\n"; continue; }

            int adminOpt = 0;
            while (adminOpt != 6) {
                adminOpt = getValidatedInt(
                    "\nAdmin Menu:\n1) View Candidates\n2) Add Candidate\n3) View Leading Party\n4) Create Election\n5) Tally Election\n6) Logout\nChoice: ", 1, 6);
                if (adminOpt == 1) { for (auto& c : candidates) c.displayInfo(); }
                else if (adminOpt == 2) {
                    string cid = getValidatedString("Enter Candidate ID: ", false);
                    string cname = getValidatedString("Enter Name: ");
                    string cpwd; cout << "Enter Password: "; getline(cin, cpwd);
                    string party = getValidatedString("Enter Party: ");
                    int exp = getValidatedInt("Enter Experience Years: ", 0, 50);
                    Candidate nc(cid, cname, cpwd, party, exp);
                    candidates.push_back(nc);
                    auth.registerUser(new Candidate(nc));
                    partyVotes[party] += 0;
                    em.registerCandidateToElection("E2025", cid);
                    cout << "Candidate added.\n";
                }
                else if (adminOpt == 3) static_cast<Admin*>(u)->showLeadingParty(partyVotes);
                else if (adminOpt == 4) {
                    string eID = getValidatedString("Enter new Election ID: ", false);
                    string eName = getValidatedString("Enter Election Name: ");
                    em.createElection(eID, eName);
                    cout << "Election created: " << eID << "\n";
                }
                else if (adminOpt == 5) {
                    string eID = getValidatedString("Enter Election ID to tally: ", false);
                    em.tally(eID);
                }
            }
        }

        else if (roleChoice == 4) {
            string id = getValidatedString("Enter Auditor ID: ", false);
            string pwd; cout << "Enter Auditor Password: "; getline(cin, pwd);
            User* u = auth.login(id, pwd);
            if (!u || u->role != "Auditor") { cout << "Wrong credentials!\n"; continue; }
            int audOpt = 0;
            while (audOpt != 3) {
                audOpt = getValidatedInt("\nAuditor Menu:\n1) View All Votes\n2) Verify Ledger\n3) Logout\nChoice: ", 1, 3);
                if (audOpt == 1) static_cast<Auditor*>(u)->viewVotes(ledger);
                else if (audOpt == 2) static_cast<Auditor*>(u)->verifyLedger(ledger);
            }
        }
    }

    // 🔹 Save candidates back to file at shutdown
    storage.saveCandidates(candidates);

    cout << "Exiting E-VotePro CLI. Goodbye!" << endl;
    return 0;
}
