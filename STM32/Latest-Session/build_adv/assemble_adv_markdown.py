import os
import re

PARTS_DIR = os.path.join(os.path.dirname(__file__), "parts")
OUTPUT_MD = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "STM32_Advanced_Topics_Complete_Guide.md"))

PART_FILES = [
    "01_front_ch1_overview.md",
    "02_topic1_flash_eeprom.md",
    "03_topic2_encoders.md",
    "04_topic3_low_power.md",
    "05_topic4_linker_startup.md",
    "06_topic5_actuators.md",
    "07_topic6_watchdogs.md",
    "08_topic7_bootloader.md",
    "09_topic8_dac.md",
    "10_case_studies.md",
    "11_reference_summary.md"
]

def make_anchor(title):
    cleaned = title.strip().lower()
    cleaned = re.sub(r'[\/\\:,\.\?\(\)\{\}\[\]\<\>\*\#\$\+\=\!\@\%\"\']', '', cleaned)
    cleaned = re.sub(r'\s+', '-', cleaned)
    return cleaned

def main():
    print("Reading advanced markdown parts...")
    all_content = []
    
    for filename in PART_FILES:
        path = os.path.join(PARTS_DIR, filename)
        if not os.path.exists(path):
            raise FileNotFoundError(f"Missing part file: {path}")
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
            all_content.append(content)
            
    full_raw_text = "\n\n".join(all_content)
    
    # Extract Headings for TOC
    toc_lines = ["## Table of Contents\n"]
    
    # Extract figures, tables, listings
    figures = []
    tables = []
    listings = []
    
    for line in full_raw_text.splitlines():
        line_s = line.strip()
        # Headings
        m = re.match(r'^(#{1,3})\s+(.+)$', line_s)
        if m:
            level = len(m.group(1))
            htitle = m.group(2).strip()
            if htitle not in ["STM32 Advanced Topics: Comprehensive Supplementary Technical Guide", 
                              "Document Information", "Table of Contents"]:
                anchor = make_anchor(htitle)
                indent = "  " * (level - 1)
                toc_lines.append(f"{indent}- [{htitle}](#{anchor})")
                
        # Figures
        mf = re.match(r'^\*Figure\s+([0-9\.]+)\s+—\s+(.+)\*$', line_s)
        if mf:
            f_num = mf.group(1)
            f_cap = mf.group(2).split(';')[0].split('.')[0].strip()
            figures.append(f"- **Figure {f_num}**: {f_cap}")
            
        # Tables
        mt = re.match(r'^\*\*Table\s+([0-9\.]+)\s+—\s+(.+)\*\*$', line_s)
        if mt:
            t_num = mt.group(1)
            t_cap = mt.group(2).strip()
            tables.append(f"- **Table {t_num}**: {t_cap}")
            
        # Listings
        ml = re.match(r'^\*\*Listing\s+([0-9\.]+)\s+—\s+(.+)\*\*$', line_s)
        if ml:
            l_num = ml.group(1)
            l_cap = ml.group(2).strip()
            listings.append(f"- **Listing {l_num}**: {l_cap}")
            
    toc_text = "\n".join(toc_lines)
    
    lists_text = "## List of Figures, Tables, and Code Listings\n\n"
    lists_text += "### Figures\n" + "\n".join(figures) + "\n\n"
    lists_text += "### Tables\n" + "\n".join(tables) + "\n\n"
    lists_text += "### Code Listings\n" + "\n".join(listings)
    
    final_text = full_raw_text.replace("<!-- TOC -->", toc_text)
    final_text = final_text.replace("<!-- LISTS -->", lists_text)
    
    with open(OUTPUT_MD, "w", encoding="utf-8") as f:
        f.write(final_text)
        
    print(f"Advanced Topics Markdown written to: {OUTPUT_MD}")
    print(f"Total lines: {len(final_text.splitlines())}, Total characters: {len(final_text)}")
    print(f"Figures extracted: {len(figures)}, Tables: {len(tables)}, Listings: {len(listings)}")

if __name__ == "__main__":
    main()
