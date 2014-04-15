#!/usr/bin/env python

import nltk
from nltk.corpus import stopwords
import string
import bs4
import re

stop_words = stopwords.words('english')

# The low end of shared words to consider
LOWER_BOUND = .20

# The high end, since anything above this is probably SEO garbage or a
# duplicate sentence
UPPER_BOUND = .90


def is_unimportant(word):
    """Decides if a word is ok to toss out for the sentence comparisons"""
    return word in ['.', '!', ',', ] or '\'' in word or word in stop_words


def only_important(sent):
    """Just a little wrapper to filter on is_unimportant"""
    return filter(lambda w: not is_unimportant(w), sent)


def compare_sents(sent1, sent2):
    """Compare two word-tokenized sentences for shared words"""
    if not len(sent1) or not len(sent2):
        return 0
    return len(set(only_important(sent1)) & set(only_important(sent2))) / ((len(sent1) + len(sent2)) / 2.0)


def compare_sents_bounded(sent1, sent2):
    """If the result of compare_sents is not between LOWER_BOUND and
    UPPER_BOUND, it returns 0 instead, so outliers don't mess with the sum"""
    cmpd = compare_sents(sent1, sent2)
    if cmpd <= LOWER_BOUND or cmpd >= UPPER_BOUND:
        return 0
    return cmpd


def compute_score(sent, sents):
    """Computes the average score of sent vs the other sentences (the result of
    sent vs itself isn't counted because it's 1, and that's above
    UPPER_BOUND)"""
    if not len(sent):
        return 0
    return sum(compare_sents_bounded(sent, sent1) for sent1 in sents) / float(len(sents))


def summarize_block(block):
    """Return the sentence that best summarizes block"""
    #print "jkj" 
    if not block:
        return None
    #print "jkj"   
    sents = nltk.sent_tokenize(block)
    word_sents = map(nltk.word_tokenize, sents)
    #print word_sents
    d = dict((compute_score(word_sent, word_sents), sent) for sent, word_sent in zip(sents, word_sents))
    return d;


def find_likely_body(b):
    """Find the tag with the most directly-descended <p> tags"""
    return max(b.find_all(), key=lambda t: len(t.find_all('p', recursive=False)))


class Summary(object):
    def __init__(self, url, article_html, title, summaries):
        self.url = url
        self.article_html = article_html
        self.title = title
        self.summaries = summaries

    def __repr__(self):
        return 'Summary({0}, {1}, {2}, {3})'.format(
                repr(self.url), repr(self.article_html), repr(self.title), repr(self.summaries)
                )

        def __str__(self):
            return u"{0} - {1}\n\n{2}".format(self.title, self.url, '\n'.join(self.summaries))


def summarize_page():

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
considering the merit achievements of AIEEE. The institute also selects students based on interviews and Kishore Vaigyanik Protsahan www.iiit.ac.in Yojana (KVPY).

Admissions for postgraduate studies are on the basis of the Postgraduate Entrance Exam (PGEE) conducted by IIIT Hyderabad.
Admissions to the MSIT programme run at this institute are based on a test conducted every year from April to May.

'''

    sentences = []
    d = summarize_block(text)
    keys = d.keys()
    keys.sort()
    keys.reverse()
    for key in keys:
        sentences.append(d[key])
    s = ""
    if len(sentences) < 4:
        length = len(sentences)
    else:
        lenght = 4
    for i in range(lenght):	
        s=s+sentences[i]	
    print s

summarize_page()
