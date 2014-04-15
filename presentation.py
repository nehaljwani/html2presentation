import latexslides
import os
import sys
sys.path.append('summarizer')
import summarize

def sanitize(text):
    """Strips newlines '\n' and '\r'"""
    text.replace('\n','')
    text.replace('\r','')
    return text

def getSections():
    arr = [
            {"title": "Titl1", "text": "text", "section": "1"},
            {"title": "Titl1", "text": "text", "section": "1"},
            {"title": "Titl1", "text": "text", "section": "2"},
            {"title": "Titl1", "text": "text", "section": "2"},
            {"title": "Titl1", "text": "text", "section": "3"},
            ]
    return arr

def joinSections(raw_sections):
    """Joins sections with same Section IDs"""
    prevID = None
    sections = []
    secText = ""
    secTitle = ""
    secID = ""
    tmpSec = {}
    for section in raw_sections:
        ID = section['section']
        text = sanitize(section['text'])
        title = sanitize(section['title'])

        if ID != prevID:
            # Dump previous section if not null
            if prevID != None:
                tmpSec['title'] = secTitle
                tmpSec['section'] = secID
                tmpSec['text'] = secText
                sections.append(tmpSec)
                tmpSec = {}

            # Now insert own data
            secText = text
            secTitle = title
            secID = ID

        else:
            # Just append text to previous section
            if text:
                secText += text
        
        prevID = ID

    # Dump remaining elements
    tmpSec['title'] = secTitle
    tmpSec['section'] = secID
    tmpSec['text'] = secText
    sections.append(tmpSec)

    return sections

def genTextSlide(ID, title, text):
    """Generates individual text slide."""
    secID = "Section " + ID
    bullets = summarize.summarize_page(text)
    slide = latexslides.BulletSlide(secID, bullets, block_heading = title)
    return slide

def genLatex(collection, filename):
    """Generates laTeX file from given collection of slides."""
    author_and_inst = [("HTML2Presentation", "IIIT Hyderabad")]

    slides = latexslides.BeamerSlides(title="Slide Ttile",
            titlepage=False,
            toc_heading=None,
            author_and_inst=author_and_inst,)

    slides.add_slides(collection)
    # Dump to file
    slides.write(filename)

def genPDF(filename):
    """Generates PDF of the TeX file whose name is supplied as argument."""
    cmd = "pdflatex " + filename + " --shell-escape"
    os.system(cmd)

def getPresentation():
    """Main function to get presentations. Wrapper for all other functions."""
    raw_sections = getSections()
    sections = joinSections(raw_sections)

    
    collection = []
    for section in sections:
        ID = section['section']
        title = section['title']
        text = section['text']

        slide = genTextSlide(ID, title, text)
        collection.append(slide)

    filename = "presentation.tex"
    genLatex(collection, filename)
    genPDF(filename)

getPresentation()
