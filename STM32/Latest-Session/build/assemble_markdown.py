import os
import re

PARTS_DIR = os.path.join(os.path.dirname(__file__), "parts")
OUTPUT_MD = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "STM32_Complete_Professional_Reference.md"))

PART_FILES = [
    "01_front_ch1.md",
    "02_ch2_workflow.md",
    "03a_ch3_irq_dma.md",
    "03b_ch3_timers.md",
    "04_ch4_protocols.md",
    "05_ch5_sensors.md",
    "06_ch6_drivers.md",
    "07_ch7_rtos.md",
    "08_ch8_capstone.md",
    "09_ch9_reference_tables.md",
    "10_appendix.md"
]

def make_anchor(title):
    # GitHub markdown anchor generator
    cleaned = title.strip().lower()
    cleaned = re.sub(r'[\/\\:,\.\?\(\)\{\}\[\]\<\>\*\#\$\+\=\!\@\%\"\']', '', cleaned)
    cleaned = re.sub(r'\s+', '-', cleaned)
    return cleaned

def main():
    print("Reading markdown parts...")
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
    headings = []
    
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
            # Skip Document Information or Table of Contents itself if already there
            if htitle not in ["STM32 Complete Professional Reference", "Document Information", "Table of Contents"]:
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
    
    # Replace markers
    final_text = full_raw_text.replace("<!-- TOC -->", toc_text)
    final_text = final_text.replace("<!-- LISTS -->", lists_text)
    
    with open(OUTPUT_MD, "w", encoding="utf-8") as f:
        f.write(final_text)
        
    print(f"Master Markdown written successfully to: {OUTPUT_MD}")
    print(f"Total lines: {len(final_text.splitlines())}, Total characters: {len(final_text)}")
    print(f"Figures extracted: {len(figures)}, Tables: {len(tables)}, Listings: {len(listings)}")

if __name__ == "__main__":
    main()
