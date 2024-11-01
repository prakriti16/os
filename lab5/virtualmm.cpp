#include <bits/stdc++.h>
using namespace std;

std::string replacementPolicy;
struct PageTableEntry {
    int frameIndex;
    bool valid;
    int currentframe;

    PageTableEntry(int fi = -1, bool v = false, int cf = -1)
        : frameIndex(fi), valid(v), currentframe(cf) {}
};

class PageTable {
public:
    explicit PageTable(size_t maxSize = INT_MAX) : maxSize(maxSize) {}
    void addEntry(uint64_t frameIndex, uint64_t currentframe);
    uint64_t getEntry(uint64_t frameIndex, PageTableEntry &entry);
    void removeEntryByFrameIndex(uint64_t frameIndex);

private:
    unordered_map<uint64_t, PageTableEntry> table;
    size_t maxSize;
};

void PageTable::addEntry(uint64_t frameIndex, uint64_t currentframe) {
    table[frameIndex] = PageTableEntry(frameIndex, true, currentframe);
}

uint64_t PageTable::getEntry(uint64_t frameIndex, PageTableEntry &entry) {
    auto it = table.find(frameIndex);
    if (it != table.end()) {
        entry = it->second;
        return it->second.currentframe;
    }
    return -1;
}

void PageTable::removeEntryByFrameIndex(uint64_t frameIndex) {
    for (auto it = table.begin(); it != table.end(); ++it) {
        if (it->second.currentframe == frameIndex) {
            table.erase(it);
            cout << "Removed entry for Frame Index: " << frameIndex << endl;
            return;
        }
    }
}

class VirtualMemoryManager {
public:
    VirtualMemoryManager(size_t pageSize, size_t numFrames)
        : pageSize(pageSize), currentframe(0), maxFrames(numFrames) {}

    void processTraceFile(const string &filePath);
    void accessMemory(int processId, uint64_t virtualAddress, size_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses);
    void removeEntry(int frameIndex);

private:
    unordered_map<int, PageTable> processPageTables;
    int currentframe;
    size_t maxFrames;
    size_t pageSize;
    queue<int> fifoQueue;
    deque<int> lruQueue;
    vector<int> usedFrames;
};

void VirtualMemoryManager::removeEntry(int frameIndex) {
    for (auto &pair : processPageTables) {
        pair.second.removeEntryByFrameIndex(frameIndex);
    }
}

void VirtualMemoryManager::processTraceFile(const string &filePath) {
    ifstream traceFile(filePath);
    if (!traceFile) {
        throw runtime_error("Error opening memory trace file: " + filePath);
    }

    string line;
    vector<pair<int, uint64_t>> futureAccesses;
    while (getline(traceFile, line)) {
        stringstream ss(line);
        int processId;
        uint64_t virtualAddress;

        string token;
        if (getline(ss, token, ',')) {
            processId = stoi(token);
        }
        if (getline(ss, token, ',')) {
            virtualAddress = stoull(token);
        }
        futureAccesses.emplace_back(processId, virtualAddress);
    }

    for (size_t i = 0; i < futureAccesses.size(); ++i) {
        accessMemory(futureAccesses[i].first, futureAccesses[i].second, i, futureAccesses);
    }
}

void VirtualMemoryManager::accessMemory(int processId, uint64_t virtualAddress, size_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses) {
    PageTableEntry entry;
    uint64_t frameIndex = virtualAddress / pageSize;

    if (processPageTables.find(processId) == processPageTables.end()) {
        processPageTables[processId] = PageTable(maxFrames);
    }

    int index = processPageTables[processId].getEntry(frameIndex, entry);
    if (index != -1) {
        cout << "Accessing existing page for Process ID: " << processId
             << ", Virtual Address: " << virtualAddress
             << ", Frame Index: " << index << endl;

        if (replacementPolicy == "lru") {
            auto it = find(lruQueue.begin(), lruQueue.end(), index);
            if (it != lruQueue.end()) {
                lruQueue.erase(it);
            }
            lruQueue.push_back(index);
        }
    } else {
        cout << "Page fault for Process ID: " << processId
             << ", Virtual Address: " << virtualAddress << endl;

        if (currentframe < maxFrames) {
            currentframe++;
            processPageTables[processId].addEntry(frameIndex, currentframe);
            fifoQueue.push(currentframe);
            lruQueue.push_back(currentframe);
            usedFrames.push_back(currentframe);
            cout << "Assigned Frame Index: " << currentframe << endl;
        } else {
            int evictedFrame;
            if (replacementPolicy == "fifo") {
                evictedFrame = fifoQueue.front();
                fifoQueue.pop();
            } else if (replacementPolicy == "lru") {
                evictedFrame = lruQueue.front();
                lruQueue.pop_front();
            } else if (replacementPolicy == "random") {
                int randomIndex = rand() % usedFrames.size();
                evictedFrame = usedFrames[randomIndex];
                usedFrames.erase(usedFrames.begin() + randomIndex);
            } else if (replacementPolicy == "optimal") {
                unordered_map<int, int> nextUse;
                for (size_t i = accessIndex + 1; i < futureAccesses.size(); ++i) {
                    int futureProcessId = futureAccesses[i].first;
                    uint64_t futureFrameIndex = futureAccesses[i].second / pageSize;

                    if (processPageTables[futureProcessId].getEntry(futureFrameIndex, entry) != -1) {
                        nextUse[entry.currentframe] = i;
                    }
                }

                int maxDistance = -1;
                for (int frame : usedFrames) {
                    int distance = nextUse.count(frame) ? nextUse[frame] - accessIndex : INT_MAX;
                    if (distance > maxDistance) {
                        maxDistance = distance;
                        evictedFrame = frame;
                    }
                }
            } else {
                cerr << "Unsupported replacement policy." << endl;
                return;
            }

            removeEntry(evictedFrame);
            processPageTables[processId].addEntry(frameIndex, evictedFrame);
            fifoQueue.push(evictedFrame);
            lruQueue.push_back(evictedFrame);
            usedFrames.push_back(evictedFrame);
            cout << "Replaced an entry and assigned Frame Index: " << evictedFrame << endl;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0]
             << " <page-size> <number-of-memory-frames> <replacement-policy> <allocation-policy> <path-to-memory-trace-file>" << endl;
        return 1;
    }

    size_t pageSize = stoul(argv[1]);
    size_t numFrames = stoul(argv[2]);
    replacementPolicy = argv[3];
    string allocationPolicy = argv[4];
    string traceFilePath = argv[5];

    if (replacementPolicy != "fifo" && replacementPolicy != "lru" && replacementPolicy != "random" && replacementPolicy != "optimal") {
        cerr << "Only FIFO, LRU, Random, and Optimal replacement policies are implemented." << endl;
        return 1;
    }

    try {
        VirtualMemoryManager vmm(pageSize, numFrames);
        vmm.processTraceFile(traceFilePath);
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
