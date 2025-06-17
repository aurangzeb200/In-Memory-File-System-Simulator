#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <ctime>  
#include <queue>

using namespace std;

class Node {
public:
    string name;
    bool isDirectory;
    string content;
    Node* firstChild;
    Node* nextSibling;
    Node* parent;

    time_t createdAt;
    time_t modifiedAt;
    size_t fileSize;
    string owner;
    unsigned int permissions;  
    bool isSymLink;
    string linkTarget;

    Node(): name(""), isDirectory(false), content(""), firstChild(nullptr), nextSibling(nullptr), parent(nullptr),
        createdAt(time(nullptr)), modifiedAt(time(nullptr)), fileSize(0), owner("root"), permissions(0755),
        isSymLink(false), linkTarget("") {}

    Node(string name, bool isDirectory, Node* parent = nullptr)
        : name(name), isDirectory(isDirectory), content(""), firstChild(nullptr), nextSibling(nullptr), parent(parent),
        createdAt(0), modifiedAt(0), fileSize(0), owner("root"), permissions(0755),
        isSymLink(false), linkTarget("") {}
};

class FileSystem {
private:
    Node* root;
    Node* currentDirectory;

    vector<string> tokenize(const string& path) {
        vector<string> tokens;
        stringstream ss(path);
        string token;
        while (getline(ss, token, '/')) {
            if (!token.empty()) tokens.push_back(token);
        }
        return tokens;
    }

    


    void deleteTree(Node* dir) {
        Node* child = dir->firstChild;
        while (child) {
            Node* next = child->nextSibling;
            if (child->isDirectory) {
                deleteTree(child);  
            }
            delete child;  
            child = next;
        }
    }

    void copyNode(Node* source, Node* destParent, const string& destName) {
        Node* copy = new Node(*source);
        copy->name = destName;

        if (source->isDirectory) {
            copy->firstChild = nullptr;
            Node* child = source->firstChild;
            while (child) {
                copyNode(child, copy, child->name);
                child = child->nextSibling;
            }
        }
        else {
            copy->content = source->content;
            copy->fileSize = source->fileSize;
        }

        copy->nextSibling = destParent->firstChild;
        destParent->firstChild = copy;
        copy->parent = destParent;
    }

    bool isCircularReference(Node* source, Node* destination) {
        while (destination) {
            if (destination == source) return true;
            destination = destination->parent;
        }
        return false;
    }

    void serializeNode(Node* node, ofstream& out) {
        if (!node) return;

        if (!node->isDirectory) {
            out << node->content << "\n";
        }

        if (node->isDirectory) {
            Node* child = node->firstChild;
            while (child) {
                serializeNode(child, out);
                child = child->nextSibling;
            }
        }
    }


    void deserializeNode(ifstream& in, Node* targetNode) {
        if (!targetNode || targetNode->isDirectory) {
            cout << "Error: Target node is invalid or a directory." << endl;
            return;
        }

        string content;
        string line;

        while (getline(in, line)) {
            content += line + "\n";
        }

        if (content.empty()) {
            cout << "Error: The file is empty or could not be read." << endl;
            return;
        }

        targetNode->content = content;
        targetNode->fileSize = content.size();  
    }




public:
    FileSystem() {
        root = new Node("/", true);
        currentDirectory = root;
    }

    ~FileSystem() {
        deleteTree(root);
    }

    bool exceedsMaxPathLength(const string& path) {
        return path.length() > 255;
    }

    Node* findNode(const string& path) {
        if (path == "/") return root;

        vector<string> tokens = tokenize(path);

        Node* node = (path[0] == '/') ? root : currentDirectory;

        for (const string& token : tokens) {
            if (token == "..") {
                if (node->parent) {
                    node = node->parent;
                }
                else {
                    return nullptr;
                }
            }
            else if (token == "." || token.empty()) {
                continue;
            }
            else {
                Node* child = node->firstChild;
                bool found = false;
                while (child) {
                    if (child->name == token) {
                        node = child;
                        found = true;
                        break;
                    }
                    child = child->nextSibling;
                }
                if (!found) return nullptr;
            }
        }

        return node;
    }

    void mkdir(const string& path) {
        if (exceedsMaxPathLength(path)) {
            cout << "Error: Path length exceeds maximum allowed length of 255 characters" << endl;
            return;
        }

        size_t lastSlash = path.find_last_of('/');
        string parentPath = path.substr(0, lastSlash);
        string dirName = path.substr(lastSlash + 1);

        Node* parent = findNode(parentPath);
        if (!parent || !parent->isDirectory) {
            cout << "Error: Invalid path" << endl;
            return;
        }

        Node* child = parent->firstChild;
        while (child) {
            if (child->name == dirName && child->isDirectory) {
                cout << "Error: Directory already exists" << endl;
                return;
            }
            child = child->nextSibling;
        }

        Node* newDir = new Node(dirName, true, parent);

        newDir->nextSibling = parent->firstChild;
        parent->firstChild = newDir;

        cout << "Directory '" << dirName << "' created successfully" << endl;
    }

    string getFullPath(Node* currentDir, const string& relativePath) {
        if (relativePath.empty()) return currentDir->name;  
        string fullPath = currentDir->name + "/" + relativePath;
        return fullPath;
    }

    void cd(const string& path) {
        if (exceedsMaxPathLength(path)) {
            cout << "Error: Path length exceeds maximum allowed length of 255 characters" << endl;
            return;
        }

        Node* node = nullptr;
        if (path[0] == '/') {
            node = findNode(path);
        }
        else {
            string fullPath = getFullPath(currentDirectory, path);  
            node = findNode(fullPath);
        }

        if (!node || !node->isDirectory) {
            cout << "Error: Invalid directory" << endl;
            return;
        }

        currentDirectory = node;  
    }

    void pwd() {
        vector<string> path;
        Node* node = currentDirectory;

        while (node) {
            path.push_back(node->name);
            node = node->parent;
        }

        if (path.empty()) {
            cout << "/";
        }
        else {
            for (auto it = path.rbegin(); it != path.rend(); ++it) {
                cout << *it;
                if (it + 1 != path.rend()) cout << "/";
            }
        }

        cout << endl;
    }

    void ls() {
        if (!currentDirectory) {
            cout << "Error: Current directory is not set" << endl;
            return;
        }

        Node* child = currentDirectory->firstChild;
        if (!child) {
            cout << "No files or directories" << endl;
            return;
        }

        while (child) {
            cout << (child->isDirectory ? "[DIR] " : "[FILE] ") << child->name << endl;
            child = child->nextSibling;
        }
    }

    void touch(const string& path, const string& content = "") {
        if (exceedsMaxPathLength(path)) {
            cout << "Error: Path length exceeds maximum allowed length of 255 characters" << endl;
            return;
        }

        size_t lastSlashPos = path.find_last_of('/');
        if (lastSlashPos == string::npos) {
            cout << "Error: Invalid path, no directory specified" << endl;
            return;
        }

        string directoryPath = path.substr(0, lastSlashPos);
        string fileName = path.substr(lastSlashPos + 1);

        if (fileName.empty()) {
            cout << "Error: Invalid file name" << endl;
            return;
        }

        Node* parent = findNode(directoryPath);
        if (!parent || !parent->isDirectory) {
            cout << "Error: Invalid directory" << endl;
            return;
        }

        Node* child = parent->firstChild;
        while (child) {
            if (child->name == fileName && !child->isDirectory) {
                cout << "Error: File already exists" << endl;
                return;
            }
            child = child->nextSibling;
        }

        Node* newFile = new Node(fileName, false, parent);
        newFile->content = content;
        newFile->fileSize = content.size();
        newFile->modifiedAt = time(0);  
        newFile->nextSibling = parent->firstChild;
        parent->firstChild = newFile;
    }

    void write(const string& fileName, const string& content) {
        Node* file = findNode(fileName);
        if (!file || file->isDirectory) {
            cout << "Error: Invalid file" << endl;
            return;
        }
        file->content = content;
        file->fileSize = content.size();
        file->modifiedAt = time(0); 
    }

    void cat(const string& fileName) {
        Node* file = findNode(fileName);
        if (!file) {
            cout << "Error: File does not exist" << endl;
            return;
        }
        if (file->isDirectory) {
            cout << "Error: " << fileName << " is a directory, not a file" << endl;
            return;
        }

        if (file->content.empty()) {
            cout << "Error: File is empty" << endl;
            return;
        }

        cout << file->content << endl;
    }

    void rm(const string& fileName) {
        if (exceedsMaxPathLength(fileName)) {
            cout << "Error: Path length exceeds maximum allowed length of 255 characters" << endl;
            return;
        }

        Node* parent = findNode(fileName.substr(0, fileName.find_last_of('/')));
        if (!parent || !parent->isDirectory) {
            cout << "Error: Invalid path" << endl;
            return;
        }

        string name = fileName.substr(fileName.find_last_of('/') + 1);
        Node* child = parent->firstChild;
        Node* prev = nullptr;

        while (child && child->name != name) {
            prev = child;
            child = child->nextSibling;
        }

        if (!child || child->isDirectory) {
            cout << "Error: File not found or it's a directory" << endl;
            return;
        }

        if (prev) {
            prev->nextSibling = child->nextSibling;  
        }
        else {
            parent->firstChild = child->nextSibling;  
        }

        delete child;  
        cout << "File " << fileName << " deleted successfully" << endl;
    }


    void mv(const string& sourcePath, const string& destPath) {
        Node* source = findNode(sourcePath);
        if (!source) {
            cout << "Error: Source path not found" << endl;
            return;
        }

        if (sourcePath == destPath) {
            cout << "Error: Source and destination are the same" << endl;
            return;
        }

        string destParentPath = destPath.substr(0, destPath.find_last_of('/'));
        Node* destParent = findNode(destParentPath);

        Node* dest = findNode(destPath);
        if (dest && dest->isDirectory) {
            destParent = dest;
        }
        else {
            if (!destParent || !destParent->isDirectory) {
                cout << "Error: Destination directory does not exist" << endl;
                return;
            }
        }

        string destName = destPath.substr(destPath.find_last_of('/') + 1);
        if (dest && dest->isDirectory) {
            destName = source->name;
        }

        Node* child = destParent->firstChild;
        while (child) {
            if (child->name == destName) {
                cout << "Error: A file or directory with the same name already exists at the destination" << endl;
                return;
            }
            child = child->nextSibling;
        }

        source->name = destName;
        source->modifiedAt = time(nullptr);  

        Node* parent = source->parent;
        if (parent->firstChild == source) {
            parent->firstChild = source->nextSibling;
        }
        else {
            Node* sibling = parent->firstChild;
            while (sibling && sibling->nextSibling != source) {
                sibling = sibling->nextSibling;
            }
            if (sibling) sibling->nextSibling = source->nextSibling;
        }

        source->nextSibling = destParent->firstChild;
        destParent->firstChild = source;
        source->parent = destParent;

        cout << "Successfully moved " << sourcePath << " to " << destPath << endl;
    }

    void cp(const string& sourcePath, const string& destPath) {
        Node* source = findNode(sourcePath);
        if (!source) {
            cout << "Error: Source path not found" << endl;
            return;
        }

        Node* dest = findNode(destPath);
        Node* destParent;
        string destName;

        if (dest && dest->isDirectory) {
            destParent = dest;
            destName = source->name;
        }
        else {
            string destParentPath = destPath.substr(0, destPath.find_last_of('/'));
            destParent = findNode(destParentPath);

            if (!destParent || !destParent->isDirectory) {
                cout << "Error: Destination path is invalid" << endl;
                return;
            }

            destName = destPath.substr(destPath.find_last_of('/') + 1);
        }

        Node* child = destParent->firstChild;
        while (child) {
            if (child->name == destName) {
                cout << "Error: A file or directory with the same name already exists at the destination" << endl;
                return;
            }
            child = child->nextSibling;
        }

        copyNode(source, destParent, destName);
        cout << "Successfully copied " << sourcePath << " to " << destPath << endl;
    }
    void stat(const string& path) {
        Node* node = findNode(path);
        if (!node) {
            cout << "Error: Path not found" << endl;
            return;
        }

        cout << "Name: " << node->name << endl;
        cout << "Type: " << (node->isDirectory ? "Directory" : "File") << endl;
        cout << "Owner: " << node->owner << endl;
        cout << "Permissions: " << oct << node->permissions << dec << endl;
        cout << "Created: " << node->createdAt << endl; 
        cout << "Modified: " << node->modifiedAt << endl; 
        if (node->isSymLink) {
            cout << "Symbolic Link Target: " << node->linkTarget << endl;
        }
        if (!node->isDirectory) {
            cout << "Size: " << node->fileSize << " bytes" << endl;
        }
    }

    void saveToFile(const string& filename) {
        ofstream out(filename); 
        if (!out) {
            cout << "Error opening file for writing." << endl;
            return;
        }

        serializeNode(root, out); 

        out.close();
        cout << "File system content saved to " << filename << endl;
    }


    void loadFromFile(const string& filename, Node* targetNode) {
        if (!targetNode || targetNode->isDirectory) {
            cout << "Error: Target node is invalid or a directory." << endl;
            return;
        }

        ifstream in(filename);
        if (!in) {
            cout << "Error: Unable to open file for reading: " << filename << endl;
            return;
        }

        deserializeNode(in, targetNode);

        in.close();
        cout << "File content successfully loaded into node: " << targetNode->name << endl;
    }



    void rename(string oldName, string newName) {
        Node* target = findNode(oldName);
        if (!target) {
            cout << "Error: File or directory not found.\n";
            return;
        }

        Node* parent = target->parent;
        Node* child = parent->firstChild;
        while (child) {
            if (child->name == newName) {
                cout << "Error: A file or directory with the new name already exists.\n";
                return;
            }
            child = child->nextSibling;
        }

        target->name = newName;
        target->modifiedAt = time(nullptr);  
        cout << "Renamed successfully.\n";
    }

    void rmdir(string path) {
        Node* target = findNode(path);
        if (!target) {
            cout << "Error: Directory not found.\n";
            return;
        }
        if (target->parent == nullptr) {
            cout << "Error: Cannot delete the root directory.\n";
            return;
        }
        Node* parent = target->parent;
        if (parent->firstChild == target) {
            parent->firstChild = target->nextSibling;
        }
        else {
            Node* sibling = parent->firstChild;
            while (sibling->nextSibling != target) {
                sibling = sibling->nextSibling;
            }
            sibling->nextSibling = target->nextSibling;
        }

        delete target;  
        cout << "Directory removed successfully.\n";
    }

    void createSymlink(const string& targetPath, const string& linkName) {
        Node* target = findNode(targetPath);
        if (!target) {
            cout << "Error: Target not found.\n";
            return;
        }

        Node* existingLink = findNode(linkName);
        if (existingLink) {
            cout << "Error: A file or symlink with the name '" << linkName << "' already exists.\n";
            return;
        }

        Node* symlink = new Node();
        symlink->name = linkName;
        symlink->isSymLink = true;
        symlink->linkTarget = targetPath;
        symlink->createdAt = symlink->modifiedAt = time(nullptr);
        symlink->parent = currentDirectory;

        if (!currentDirectory->firstChild) {
            currentDirectory->firstChild = symlink;
        }
        else {
            Node* sibling = currentDirectory->firstChild;
            while (sibling->nextSibling) {
                sibling = sibling->nextSibling;
            }
            sibling->nextSibling = symlink;
        }

        cout << "Symbolic link '" << linkName << "' created successfully, pointing to '" << targetPath << "'.\n";
    }

    void chmod(const string& path, unsigned int mode) {
        Node* target = findNode(path);
        if (!target) {
            cout << "Error: File or directory not found.\n";
            return;
        }

        target->permissions = mode;
        target->modifiedAt = time(nullptr);

        cout << "Permissions for '" << path << "' updated successfully.\n";
    }

    void chown(const string& path, const string& newOwner) {
        Node* target = findNode(path);
        if (!target) {
            cout << "Error: File or directory not found.\n";
            return;
        }

        target->owner = newOwner;
        target->modifiedAt = time(nullptr);

        cout << "Ownership of '" << path << "' updated successfully to '" << newOwner << "'.\n";
    }

    string toLower(const string& str) {
        string result = str;
        for (char& ch : result) {
            if (ch >= 'A' && ch <= 'Z') {
                ch = ch + 32;
            }
        }
        return result;
    }

    string constructPath(Node* node) {
        if (!node) return "";

        vector<string> pathParts;
        while (node) {
            pathParts.push_back(node->name);
            node = node->parent;
        }

        reverse(pathParts.begin(), pathParts.end()); 

        string fullPath = "/";
        for (const string& part : pathParts) {
            fullPath += part + "/";
        }
        return fullPath.substr(0, fullPath.size() - 1);
    }

    void find(const string& pattern) {
        if (!currentDirectory) {
            cout << "Error: Current directory is null.\n";
            return;
        }

        vector<Node*> results;
        queue<Node*> nodesToProcess;
        nodesToProcess.push(currentDirectory);

        while (!nodesToProcess.empty()) {
            Node* node = nodesToProcess.front();
            nodesToProcess.pop();

            if (!node) continue;

            if (node->name.find(pattern) != string::npos) { 
                results.push_back(node);
            }

            if (node->firstChild) {
                nodesToProcess.push(node->firstChild);
            }
            if (node->nextSibling) {
                nodesToProcess.push(node->nextSibling);
            }
        }

        if (results.empty()) {
            cout << "No matches found.\n";
        }
        else {
            for (Node* result : results) {
                cout << constructPath(result) << " (" << (result->isDirectory ? "directory" : "file") << ")\n";
            }
        }
    }

    void findInsensitive(const string& pattern) {
        if (!currentDirectory) {
            cout << "Error: Current directory is null.\n";
            return;
        }

        vector<Node*> results;
        queue<Node*> nodesToProcess;
        nodesToProcess.push(currentDirectory);

        string searchPattern = toLower(pattern);

        while (!nodesToProcess.empty()) {
            Node* node = nodesToProcess.front();
            nodesToProcess.pop();

            if (!node) continue;

            string nodeName = toLower(node->name); 

            if (nodeName.find(searchPattern) != string::npos) { 
                results.push_back(node);
            }

            if (node->firstChild) {
                nodesToProcess.push(node->firstChild);
            }
            if (node->nextSibling) {
                nodesToProcess.push(node->nextSibling);
            }
        }

        if (results.empty()) {
            cout << "No matches found.\n";
        }
        else {
            for (Node* result : results) {
                cout << constructPath(result) << " (" << (result->isDirectory ? "directory" : "file") << ")\n";
            }
        }
    }

    void grep(const string& content) {
        vector<Node*> results;
        queue<Node*> nodesToProcess;

        if (!currentDirectory) {
            cout << "Error: Current directory is null.\n";
            return;
        }

        nodesToProcess.push(currentDirectory);

        while (!nodesToProcess.empty()) {
            Node* node = nodesToProcess.front();
            nodesToProcess.pop();

            if (!node) continue;

            if (!node->isDirectory && node->content.find(content) != string::npos) {
                results.push_back(node);
            }

            if (node->firstChild) {
                nodesToProcess.push(node->firstChild);
            }
            if (node->nextSibling) {
                nodesToProcess.push(node->nextSibling);
            }
        }

        if (results.empty()) {
            cout << "No files contain the specified content.\n";
        }
        else {
            for (Node* result : results) {
                cout << "File: " << result->name << " contains the specified content.\n";
            }
        }
    }
};

void executeCommand(const string& command, FileSystem& fs) {
    stringstream ss(command);
    string cmd;
    ss >> cmd;

    if (cmd == "mkdir") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.mkdir(path);
        }
    }
    else if (cmd == "cd") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.cd(path);
        }
    }
    else if (cmd == "pwd") {
        fs.pwd();
    }
    else if (cmd == "ls") {
        fs.ls();
    }
    else if (cmd == "touch") {
        string path, content;
        ss >> path;
        getline(ss, content); 
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.touch(path, content.substr(1)); 
        }
    }
    else if (cmd == "write") {
        string path, content;
        ss >> path;
        getline(ss, content); 
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.write(path, content.substr(1)); 
        }
    }
    else if (cmd == "cat") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.cat(path);
        }
    }
    else if (cmd == "rm") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.rm(path);
        }
    }
    else if (cmd == "mv") {
        string src, dest;
        ss >> src >> dest;
        if (src.empty() || dest.empty()) {
            cout << "Error: Source or destination path is missing" << endl;
        }
        else {
            fs.mv(src, dest);
        }
    }
    else if (cmd == "cp") {
        string src, dest;
        ss >> src >> dest;
        if (src.empty() || dest.empty()) {
            cout << "Error: Source or destination path is missing" << endl;
        }
        else {
            fs.cp(src, dest);
        }
    }
    else if (cmd == "stat") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.stat(path);
        }
    }
    else if (cmd == "save") {
        string filename;
        ss >> filename;
        if (filename.empty()) {
            cout << "Error: Filename is missing" << endl;
        }
        else {
            fs.saveToFile(filename);
        }
    }
    else if (cmd == "load") {
        string filename, targetPath;
        ss >> filename >> targetPath;

        if (filename.empty()) {
            cout << "Error: Filename is missing." << endl;
        }
        else if (targetPath.empty()) {
            cout << "Error: Target path is missing." << endl;
        }
        else {
            Node* targetNode = fs.findNode(targetPath);

            if (!targetNode) {
                cout << "Error: Node at path '" << targetPath << "' not found." << endl;
            }
            else if (targetNode->isDirectory) {
                cout << "Error: Cannot load content into a directory." << endl;
            }
            else {
                fs.loadFromFile(filename, targetNode);
                cout << "Content from '" << filename << "' successfully loaded into node: " << targetPath << endl;
            }
        }
        }




    else if (cmd == "rename") {
        string oldName, newName;
        ss >> oldName >> newName;
        if (oldName.empty() || newName.empty()) {
            cout << "Error: Old or new name is missing" << endl;
        }
        else {
            fs.rename(oldName, newName);
        }
    }
    else if (cmd == "rmdir") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.rmdir(path);
        }
    }
    else if (cmd == "createSymlink") {
        string target, linkName;
        ss >> target >> linkName;
        if (target.empty() || linkName.empty()) {
            cout << "Error: Target or link name is missing" << endl;
        }
        else {
            fs.createSymlink(target, linkName);
        }
    }
    else if (cmd == "chmod") {
        string path;
        unsigned int permissions;
        ss >> path >> permissions;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.chmod(path, permissions);
        }
    }
    else if (cmd == "chown") {
        string path, owner;
        ss >> path >> owner;
        if (path.empty() || owner.empty()) {
            cout << "Error: Path or owner is missing" << endl;
        }
        else {
            fs.chown(path, owner);
        }
    }
    else if (cmd == "toLower") {
        string input;
        ss >> input;
        if (input.empty()) {
            cout << "Error: Input is missing" << endl;
        }
        else {
            fs.toLower(input);
        }
    }
    else if (cmd == "find") {
        string path;
        ss >> path;
        if (path.empty()) {
            cout << "Error: Path is missing" << endl;
        }
        else {
            fs.find(path);
        }
    }
    else if (cmd == "grep") {
        string pattern;
        ss >> pattern;
        if (pattern.empty()) {
            cout << "Error: Pattern or path is missing" << endl;
        }
        else {
            fs.grep(pattern);
        }
    }
    else {
        cout << "Error: Unknown command" << endl;
    }
}

void startCLI(FileSystem& fs) {
    while (true) {
        cout << "> ";
        string command;
        getline(cin, command);

        if (command == "exit") {
            cout << "Exiting file system CLI." << endl;
            break;
        }

        if (command.empty()) {
            continue;
        }

        executeCommand(command, fs);
    }
}

int main() {
    FileSystem fs;
    startCLI(fs);
    return 0;
}
