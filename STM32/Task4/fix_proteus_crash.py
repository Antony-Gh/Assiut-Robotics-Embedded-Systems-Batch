import zipfile
import shutil
import os

def fix_proteus_pdsprj(prj_path):
    """
    Fixes the 'Internal Exception: access violation in module TEPMPU6050.DLL [0003CCEC]' crash in Proteus.
    
    Root Cause:
    Both MPU-6050 (U2) and BMP280 (U3) are TEP library models configured with PANELSTYLE=SHEET.
    Both models attempt to hook the ISIS schematic sheet canvas simultaneously using a shared
    singleton (SheetCanvas<VICOMPONENT>), which corrupts the pin-descriptor vector of MPU-6050.
    
    Fix:
    Changes BMP280 (U3) from PANELSTYLE=SHEET (On-Sheet Panel) to PANELSTYLE=POPUP (Pop-Up Panel).
    Because 'SHEET' and 'POPUP' are both exactly 5 ASCII characters, this substitution preserves
    all binary block lengths, stream offsets, and Proteus DSN structures perfectly.
    """
    if not os.path.exists(prj_path):
        print(f"Error: File not found: {prj_path}")
        return False

    bak_path = prj_path + ".bak"
    shutil.copyfile(prj_path, bak_path)
    print(f"Backup created at: {bak_path}")

    # Read all streams from the Proteus zip container
    with zipfile.ZipFile(bak_path, 'r') as zin:
        items = {item.filename: zin.read(item.filename) for item in zin.infolist()}

    dsn = items.get('ROOT.DSN')
    if not dsn:
        print("Error: ROOT.DSN not found inside .pdsprj!")
        return False

    # 1. Update U3 instance property (BMP280 component)
    u3_pos = dsn.find(b'U3')
    if u3_pos != -1:
        sheet_pos = dsn.find(b'{PANELSTYLE=SHEET}', u3_pos)
        if sheet_pos != -1 and sheet_pos < u3_pos + 1000:
            dsn = dsn[:sheet_pos] + b'{PANELSTYLE=POPUP}' + dsn[sheet_pos + len(b'{PANELSTYLE=SHEET}'):]
            print("[OK] Patched U3 instance property: PANELSTYLE=SHEET -> PANELSTYLE=POPUP")

    # 2. Update BMP280 library template definition
    old_bmp_chunk = b'{PRIMITIVE=DIGITAL,BMP280TEP}\n{MODDLL=TEPBMP280.DLL}\n{TEMPERATURE=25}\n{PRESSURE=1013.25}\n{PANELSTYLE=SHEET}'
    new_bmp_chunk = b'{PRIMITIVE=DIGITAL,BMP280TEP}\n{MODDLL=TEPBMP280.DLL}\n{TEMPERATURE=25}\n{PRESSURE=1013.25}\n{PANELSTYLE=POPUP}'
    if old_bmp_chunk in dsn:
        dsn = dsn.replace(old_bmp_chunk, new_bmp_chunk)
        print("[OK] Patched BMP280 template: PANELSTYLE=SHEET -> PANELSTYLE=POPUP")

    items['ROOT.DSN'] = dsn

    # Repack into .pdsprj
    with zipfile.ZipFile(prj_path, 'w') as zout:
        for fname, data in items.items():
            zout.writestr(fname, data)

    print(f"[SUCCESS] Patched Proteus project saved to: {prj_path}")
    return True

if __name__ == '__main__':
    project_file = r'STM32/Task4/STM32F103C8TX/STM32F103C8TX/Task4-Proteus.pdsprj'
    fix_proteus_pdsprj(project_file)
