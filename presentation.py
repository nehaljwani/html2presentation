#!/usr/bin/env python

import json
import latexslides
import os
import sys
sys.path.append('summarizer')
import summarize

tables = []
images = []

def sanitize(text):
    """Strips newlines '\n', '\t' and '\r'."""
    text.replace('\n',' ')
    text.replace('\r','')
    text.replace('\t',' ')
    return text

def runParser(pathToPaper):
    """Executes the HTML parser."""
    cmd = "parser/parser " + pathToPaper + " > sections.json"
    os.system(cmd)

def getSections(filename, pathToPaper):
    runParser(pathToPaper)
    lines = open(filename).read()
    sections = json.loads(lines,  "ISO-8859-1")
    return sections

def joinSections(raw_sections):
    """Joins sections with same Section IDs"""
    prevID = None
    sections = []
    secText = ""
    secTitle = ""
    secID = ""
    tmpSec = {}
    for section in raw_sections:
        # Check for tables
        if 'table' in section:
            tables.append(section)
            continue

        ID = section['section']
        title = sanitize(section['title'])

        # Check for images
        if 'attr' in section:
            if section['attr'] == 'img':
                images.append(section)
                continue

        if 'text' in section:
            text = sanitize(section['text'])
        else:
            text = ""

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
                secText += " " + text
        
        prevID = ID

    # Dump remaining elements
    tmpSec['title'] = secTitle
    tmpSec['section'] = secID
    tmpSec['text'] = secText
    sections.append(tmpSec)

    return sections

def genTitleSlide(title):
    """Generates the title slide that appears on first page."""
    slide = latexslides.TextSlide(
            title = title,
            block_heading = title,
            )
    return slide

def genTextSlide(ID, title, text):
    """Generates individual text slide."""
    secID = ID
    if text == None or text == "":
        text = "No Description available."
    bullets = summarize.summarize_page(text)
    slide = latexslides.BulletSlide(secID, bullets, block_heading = title)
    return slide

def convertToJPG(path):
    extension = path.split('.')[-1]
    if extension == 'gif':
        cmd = 'i=' + path + ';convert $i ${i%.gif}.jpg'
        os.system(cmd)
        newPath = path[:-4] + '.jpg'
        return newPath
    else:
        return path

def genImgSlide(image):
    slide = latexslides.Slide(image['title'],
            figure=convertToJPG(image['path']),
            figure_pos='w',
            figure_fraction_width=0.3,
            left_column_width=0.8)
    return slide

def extractTable(rawTable):
    table = []

    for row in rawTable:
        tmpRow = []
        for col in row:
            tmpCol = ""
            for element in col:
                if 'text' in element and 'attr' not in element:
                    tmpCol += element['text']
            if tmpCol == "":
                return []
            tmpRow.append(tmpCol)
        table.append(tmpRow)

    return table

def genTableSlide(table):
    formattedTable = extractTable(table['table'])
    if formattedTable == []:
        return None
    slide = latexslides.TableSlide(
            title = table['section'],
            table = formattedTable,
            block_heading = table['title'],
            )
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
    newlines = 'echo -e "' + '\\n' * 100 + '"' # Hack to continue when pdflatex halts.
    cmd = newlines + " | pdflatex " + filename + " --shell-escape 2>/dev/null >/dev/null"
    os.system(cmd)

def getPresentation():
    """Main function to get presentations. Wrapper for all other functions."""
    if len(sys.argv) != 2:
        return "Proper arguments not provided. Aborting!!"

    pathToPaper = sys.argv[1]

    if os.path.isfile(pathToPaper) is False:
        return "Specified file does not exists. Aborting!!"

    raw_sections = getSections("sections.json", pathToPaper)
    sections = joinSections(raw_sections)

    collection = []
    for section in sections:
        ID = section['section']
        title = section['title']
        text = section['text']

        if ID == "Section: 0":
            slide = genTitleSlide(title)
        else:
            slide = genTextSlide(ID, title, text)
        collection.append(slide)

        # Check if any images exist for this section.
        for image in images:
            if image['section'] == ID:
                #image belongs to this section. Add its slide
                slide = genImgSlide(image)
                collection.append(slide)
        
        # Check if any tables exist for this section.
        for table in tables:
            if table['section'] == ID:
                #table belongs to this section. Add its slide
                slide = genTableSlide(table)
                if slide is not None:
                    collection.append(slide)


    filename = "presentation.tex"
    genLatex(collection, filename)
    genPDF(filename)
    
    return "PDF successfully printed to `presentation.pdf`."

if __name__ == "__main__":
    print getPresentation()
