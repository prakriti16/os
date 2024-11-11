#include <bits/stdc++.h>
using namespace std;

string replacementPolicy;
string allocationPolicy;
long long pagefaults=0;
struct PageTableEntry {
    uint64_t pageIndex;
    bool valid;
    uint64_t currentframe;

    PageTableEntry(uint64_t pi = UINT64_MAX, bool v = false, uint64_t cf = UINT64_MAX)
        : pageIndex(pi), valid(v), currentframe(cf) {}
};

class PageTable {
public:
    explicit PageTable(uint64_t maxSize = INT_MAX) : maxSize(maxSize) {}
    void addEntry(uint64_t pageIndex, uint64_t currentframe);
    uint64_t getEntry(uint64_t pageIndex, PageTableEntry &entry);
    void removeEntryByFrameIndex(uint64_t frameIndex);

private:
    unordered_map<uint64_t, PageTableEntry> table;
    uint64_t maxSize;
};

void PageTable::addEntry(uint64_t pageIndex, uint64_t currentframe) {
    table[pageIndex] = PageTableEntry(pageIndex, true, currentframe);
}

uint64_t PageTable::getEntry(uint64_t pageIndex, PageTableEntry &entry) {
    auto it = table.find(pageIndex);
    if (it != table.end()) {
        entry = it->second;//entry from page table having fi, v,cf
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
    VirtualMemoryManager(uint64_t pageSize, uint64_t numFrames)
        : pageSize(pageSize), currentframe(0), maxFrames(numFrames) {}

    void processTraceFile(const string &filePath);
    void accessMemory(int processId, uint64_t virtualAddress, uint64_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses);
    void removeEntry(int frameIndex);

private:
    unordered_map<int, PageTable> processPageTables;
    int currentframe;
    uint64_t maxFrames;
    uint64_t pageSize;
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

    for (uint64_t i = 0; i < futureAccesses.size(); ++i) {
        accessMemory(futureAccesses[i].first, futureAccesses[i].second, i, futureAccesses);
    }
}

void VirtualMemoryManager::accessMemory(int processId, uint64_t virtualAddress, uint64_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses) {
    PageTableEntry entry;
    uint64_t pageIndex = virtualAddress / pageSize;
    if (processPageTables.find(processId) == processPageTables.end()) {
        processPageTables[processId] = PageTable(maxFrames);
    }

    int index = processPageTables[processId].getEntry(pageIndex, entry);//index is frame index
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
        pagefaults++;
        cout<<pagefaults;

        if (currentframe < maxFrames) {
            currentframe++;
            processPageTables[processId].addEntry(pageIndex, currentframe);
            fifoQueue.push(currentframe);
            lruQueue.push_back(currentframe);
            usedFrames.push_back(currentframe);
            cout << "Assigned Frame Index: " << currentframe << " and page index: "<<pageIndex << endl;
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
                for (uint64_t i = accessIndex + 1; i < futureAccesses.size(); ++i) {
                    int futureProcessId = futureAccesses[i].first;
                    uint64_t futurePageIndex = futureAccesses[i].second / pageSize;

                    if (processPageTables[futureProcessId].getEntry(futurePageIndex, entry) != -1) {
                        // Check if the current frame is already in the nextUse map
                        auto it = nextUse.find(entry.currentframe);
                        
                        // If the frame is not already in the map, update it 
                        if (it == nextUse.end()) {
                            nextUse[entry.currentframe] = i;
                        }
                        //ensures that earliest access to frame is preserved
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
            processPageTables[processId].addEntry(pageIndex, evictedFrame);
            fifoQueue.push(evictedFrame);
            lruQueue.push_back(evictedFrame);
            usedFrames.push_back(evictedFrame);
            cout << "Replaced an entry and assigned Frame Index: " << evictedFrame << endl;
        }
    }
}
class VirtualMemoryManagerL {
public:
    VirtualMemoryManagerL(uint64_t pageSize, uint64_t numFrames, uint64_t numProcesses)
        : pageSize(pageSize), currentframe(0), maxFrames(numFrames), numProcesses(numProcesses) {
        // Initialize frame allocation for each process
        allocateFramesToProcesses();
    }

    void processTraceFile(const string &filePath);
    void accessMemory(int processId, uint64_t virtualAddress, uint64_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses);
    void removeEntry(int frameIndex);
    void assignFrameToProcess(int processId, uint64_t pageIndex); // Declaration for assignFrameToProcess
    bool isFrameFree(int frame); // Declaration for isFrameFree
    int evictFrameFromRange(int processId); // Declaration for evictFrameFromRange


private:
    unordered_map<int, PageTable> processPageTables;
    unordered_map<int, pair<int, int>> processFrameRange; // Process ID -> (startFrame, endFrame)
    int currentframe;
    uint64_t maxFrames;
    uint64_t pageSize;
    uint64_t numProcesses;
    queue<int> fifoQueue;
    deque<int> lruQueue;
    vector<int> usedFrames;

    void allocateFramesToProcesses();
    bool isFrameAllocatedToProcess(int processId, int frame);
};

void VirtualMemoryManagerL::allocateFramesToProcesses() {
    int framesPerProcess = maxFrames / numProcesses;
    int remainingFrames = maxFrames % numProcesses;
    int startFrame = 0;

    // Allocate frames to each process
    for (int processId = 0; processId < numProcesses; ++processId) {
        int endFrame = startFrame + framesPerProcess - 1;
        if (remainingFrames > 0) {
            endFrame++;
            remainingFrames--;
        }
        processFrameRange[processId] = {startFrame, endFrame};
        cout<<"s"<<startFrame<<"e"<<endFrame<<endl;
        startFrame = endFrame + 1;
    }
}

bool VirtualMemoryManagerL::isFrameAllocatedToProcess(int processId, int frame) {
    auto it = processFrameRange.find(processId);
    if (it != processFrameRange.end()) {
        return frame >= it->second.first && frame <= it->second.second;
    }
    return false;
}

void VirtualMemoryManagerL::removeEntry(int frameIndex) {
    for (auto &pair : processPageTables) {
        pair.second.removeEntryByFrameIndex(frameIndex);
    }
}

void VirtualMemoryManagerL::processTraceFile(const string &filePath) {
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

    for (uint64_t i = 0; i < futureAccesses.size(); ++i) {
        accessMemory(futureAccesses[i].first, futureAccesses[i].second, i, futureAccesses);
    }
}

// Example frame assignment function inside your memory manager
void VirtualMemoryManagerL::assignFrameToProcess(int processId, uint64_t pageIndex) {
    bool frameAssigned = false;
    auto &frameRange = processFrameRange[processId];

    // Check for a free frame within the allocated range
    for (int frame = frameRange.first; frame <= frameRange.second; ++frame) {
        if (isFrameFree(frame)) {
            // Frame is free, assign it to the process
            processPageTables[processId].addEntry(pageIndex, frame);
            PageTableEntry entry;
            processPageTables[processId].getEntry(pageIndex, entry);
            cout << "free entry Page Index: " << static_cast<uint64_t>(entry.pageIndex) << endl;
            cout << "free entry Valid: " << (entry.valid ? "true" : "false") << endl;
            cout << "free entry Current Frame: " << entry.currentframe << endl;
            fifoQueue.push(frame);
            lruQueue.push_back(frame);
            usedFrames.push_back(frame);
            frameAssigned = true;
            cout << "Assigned Frame Index: " << frame << " and page index: " << pageIndex << endl;
            break;
        }
    }

    // If no free frame is found, evict a frame within the allocated range
    if (!frameAssigned) {
        int evictedFrame = evictFrameFromRange(processId); // Eviction logic within range
        removeEntry(evictedFrame);
        processPageTables[processId].addEntry(pageIndex, evictedFrame);
        PageTableEntry entry;
        processPageTables[processId].getEntry(pageIndex, entry);
        // cout << "Eviction entry Page Index: " << static_cast<uint64_t>(entry.pageIndex) << endl;
        // cout << "Eviction entry Valid: " << (entry.valid ? "true" : "false") << endl;
        // cout << "Eviction entry Current Frame: " << entry.currentframe << endl;
        fifoQueue.push(evictedFrame);
        lruQueue.push_back(evictedFrame);
        usedFrames.push_back(evictedFrame);
        cout << "Evicted Frame Index: " << evictedFrame << " and assigned to page index: " << pageIndex << endl;
    }
}

// Function to check if a frame is free
bool VirtualMemoryManagerL::isFrameFree(int frame) {
    return find(usedFrames.begin(), usedFrames.end(), frame) == usedFrames.end();
}

// Function to evict a frame within a process's allocated range
int VirtualMemoryManagerL::evictFrameFromRange(int processId) {
    auto &frameRange = processFrameRange[processId];
    int evictedFrame = -1;
    if (replacementPolicy == "fifo") {
        queue<int> tempQueue;
        // Evict the oldest frame from the FIFO queue within the range
        while (!fifoQueue.empty()) {
            evictedFrame = fifoQueue.front();
            fifoQueue.pop();
            if (evictedFrame >= frameRange.first && evictedFrame <= frameRange.second) {
                
                break;
            }
            else {
                tempQueue.push(evictedFrame);  // Keep frames out of range
            }
        }
        while (!tempQueue.empty()) {
            fifoQueue.push(tempQueue.front());
            tempQueue.pop();
        }
    } else if (replacementPolicy == "lru") {
        list<int> tempQueue;
        // Evict the least recently used frame from the LRU queue within the range
        while (!lruQueue.empty()) {
            evictedFrame = lruQueue.front();
            lruQueue.pop_front();
            if (evictedFrame >= frameRange.first && evictedFrame <= frameRange.second) {
                
                break;
            } else {
                tempQueue.push_back(evictedFrame);  // Keep frames out of range
            }
        }
        for (int frame : tempQueue) {
            lruQueue.push_back(frame);
        }
    }
    if (evictedFrame == -1) {
        cerr << "Error: No frame available for eviction in the specified range for Process ID: " << processId << endl;
    }
    return evictedFrame;
}

void VirtualMemoryManagerL::accessMemory(int processId, uint64_t virtualAddress, uint64_t accessIndex, const vector<pair<int, uint64_t>>& futureAccesses) {
    PageTableEntry entry;
    uint64_t pageIndex = static_cast<uint64_t>(virtualAddress) / pageSize;
    // cout << "Virtual Address: " << virtualAddress << ", Page Size: " << pageSize << endl;
    // uint64_t pageIndex = virtualAddress / pageSize;
    // cout << "Calculated Page Index: " << pageIndex << endl;

    if (processPageTables.find(processId) == processPageTables.end()) {
        processPageTables[processId] = PageTable(maxFrames);
    }

    int index = processPageTables[processId].getEntry(pageIndex, entry);
    if (index != -1) {
        // cout << "entry Page Index: " << static_cast<uint64_t>(entry.pageIndex) << endl;
        // cout << "entry Valid: " << (entry.valid ? "true" : "false") << endl;
        // cout << "entry Current Frame: " << entry.currentframe << endl;
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
        pagefaults++;
        cout << pagefaults;
        assignFrameToProcess(processId, pageIndex);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0]
             << " <page-size> <number-of-memory-frames> <replacement-policy> <allocation-policy> <path-to-memory-trace-file>" << endl;
        return 1;
    }

    uint64_t pageSize = stoul(argv[1]);
    uint64_t numFrames = stoul(argv[2]);
    replacementPolicy = argv[3];
    allocationPolicy = argv[4];
    string traceFilePath = argv[5];

    if (replacementPolicy != "fifo" && replacementPolicy != "lru" && replacementPolicy != "random" && replacementPolicy != "optimal") {
        cerr << "Only FIFO, LRU, Random, and Optimal replacement policies are implemented." << endl;
        return 1;
    }

    if (allocationPolicy != "local" && allocationPolicy != "global") {
        cerr << "Only local and global allocation policies are implemented." << endl;
        return 1;
    }   
    if (allocationPolicy == "global"){
        try {
            
            VirtualMemoryManager vmm(pageSize, numFrames);
            vmm.processTraceFile(traceFilePath);
            cout << "Page faults: " << pagefaults << endl;
        } catch (const exception &e) {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }
    else{
        if(numFrames<4){
            cerr << "Number of frames should be greater than 4." << endl;
            return 1;
        }
        try {
            uint64_t numProcesses = 4;
            VirtualMemoryManagerL vmm(pageSize, numFrames, numProcesses);
            vmm.processTraceFile(traceFilePath);
            cout << "Page faults: " << pagefaults << endl;
        } catch (const exception &e) {
            cerr << "Error: " << e.what() << endl;
            return 1;
        }
    }
    

    return 0;
}
