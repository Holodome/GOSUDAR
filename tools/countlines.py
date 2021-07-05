import os 

total_lines = 0
total_files = 0
SOURCE_FOLDER = "src"
for file in os.listdir(SOURCE_FOLDER):
    if not file.endswith("hh") and not file.endswith("cc"):
        continue
    
    file_path = os.path.join(SOURCE_FOLDER, file)
    file_lines = len(open(file_path).readlines())
    total_lines += file_lines
    total_files += 1
    print(f"{file}: {file_lines}")
    
print(f"Files: {total_files}; Lines: {total_lines}")