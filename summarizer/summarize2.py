#!/usr/bin/env python



from collections import defaultdict
import re

MAX_SUMMARY_SIZE = 300


def tokenize(text):

    return text.split()


def split_to_sentences(text):

    sentences = []
    start = 0


    for match in re.finditer('(\s*[.!?]\s*)|(\n{2,})', text):
        sentences.append(text[start:match.end()].strip())
        start = match.end()

    if start < len(text):
        sentences.append(text[start:].strip())

    return sentences


def token_frequency(text):

    frequencies = defaultdict(int)
    for token in tokenize(text):
        frequencies[token] += 1

    return frequencies


def sentence_score(sentence, frequencies):
    return sum((frequencies[token] for token in tokenize(sentence)))


def create_summary(sentences, max_length):
    summary = []
    size = 0
    for sentence in sentences:
        summary.append(sentence)
        #size += len(sentence)
        size += 1
        if size >= max_length:
            break

    summary = summary[:max_length]
    return '\n'.join(summary)


def summarize(text, max_summary_size=MAX_SUMMARY_SIZE):
    frequencies = token_frequency(text)
    sentences = split_to_sentences(text)
    sentences.sort(key=lambda s: sentence_score(s, frequencies), reverse=1)
    summary = create_summary(sentences, max_summary_size)

    return summary


def _test(size=10):
    text = '''
    The International Institute of Information Technology, Hyderabad (IIIT Hyderabad) is an autonomous university in Hyderabad, Andhra Pradesh, India.
    It was established in 1998, and is one of the most prestigious institutes of the country.
    It emphasizes research from the undergraduate level, which makes it different from the other leading engineering institutes in India.
    It has been a consistent performer from India in ACM International Collegiate Programming Contest (ICPC) and finished at #18 last year.[1] Raj Reddy,
    \the only Indian to win the Turing Award, is chairman of the board of governors.

The institute runs CS courses and research projects and is focused on research.
It gives the students interaction with industry, preparation in entrepreneurship and personality development courses.
IIIT Hyderabad the mentor institute to Indian Institute of Information Technology, Sri City[2]
IIIT Hyderabad was set up in 1998.
It was envisioned by Nara Chandrababu Naidu, Chief Minister of the Andhra Pradesh from 1995 to 2004.
The government of Andhra Pradesh lent support to the institute by grant of land and buildings.
 A Governing Council consisting of people from academia, industry and government presides over the governance of the institution.
  Rajeev Sangal, former Head of Department of Computer Science, IIT Kanpur, designed the syllabus. He was the director of the institute
  till April 10 2013. P. J. Narayanan, the former Dean R&D and current President of Association for Computing Machinery (ACM India) is the current Director.
Admissions for undergraduate and dual degrees were taken through AIEEE i.e. CCB (Central Counseling board) counseling until the year 2009.

From the academic batch 2010, the institute revised the admission procedure and decided on independent selection students though still
considering the merit achievements of AIEEE. The institute also selects students based on interviews and Kishore Vaigyanik Protsahan Yojana (KVPY).

Admissions for postgraduate studies are on the basis of the Postgraduate Entrance Exam (PGEE) conducted by IIIT Hyderabad.
Admissions to the MSIT programme run at this institute are based on a test conducted every year from April to May.

'''

    print(summarize(text, size))



_test(5)

#print summarize(open("/tmp/out").read())
