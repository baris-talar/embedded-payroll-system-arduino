Embedded Payroll System (Arduino + LCD UI)

Overview

This project is a payroll management system built on an Arduino using an RGB LCD shield. It allows managing employee records through both physical buttons and serial commands.

The system is designed to work within microcontroller constraints such as limited memory and display size.

⸻

Features
	•	Add, update, and delete employee accounts
	•	Modify:
	•	Salary
	•	Grade
	•	Job title
	•	Pension status
	•	LCD-based user interface (16x2 display)
	•	Button navigation
	•	Serial command interface
	•	Scrollable display for long text

⸻

Tech Stack
	•	Arduino (C++)
	•	I2C communication (Wire.h)
	•	Adafruit RGB LCD Shield

  System Design
	•	Stores up to 10 employee accounts
	•	Uses a structured data model for each employee
	•	Implements a simple state-based UI for navigation
	•	Handles input via both hardware buttons and serial communication
