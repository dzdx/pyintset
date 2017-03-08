SHELL := /bin/bash

test:
	python setup.py install
	python -m unittest intset_test
