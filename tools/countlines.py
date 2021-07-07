import os 

total_lines = 0
non_blank_lines = 0
total_files = 0
SOURCE_FOLDER = "src"
for file in os.listdir(SOURCE_FOLDER):
    if not file.endswith("hh") and not file.endswith("cc"):
        continue
    
    file_path = os.path.join(SOURCE_FOLDER, file)
    lines = open(file_path).readlines()
    file_lines = len(lines)
    file_non_blank_lines = len(list(filter(lambda line: list(filter(lambda char: not char.isspace(), line)), lines)))
    total_lines += file_lines
    non_blank_lines += file_non_blank_lines
    total_files += 1
    print(f"{file}: {file_lines} ({file_non_blank_lines})")
    
print(f"Files: {total_files}; Lines: {total_lines}; Non blank lines: {non_blank_lines}")