import sys
try:
    import pymupdf as fitz
    doc = fitz.open('Encomienda_Valdez_PEMBEDS_ProjectProposal_Template.pdf')
    text = ""
    for page in doc:
        text += page.get_text()
    with open('output.txt', 'w', encoding='utf-8') as f:
        f.write(text)
    print("Success")
except Exception as e:
    print("Failed to read: ", e)
