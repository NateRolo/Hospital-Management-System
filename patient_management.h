/*
 * Author: Arsh M, Nathan O
 * Date: Feb 6, 2025
 * Purpose: This file defines functions for managing patient records in a hospital system.
 */

#ifndef PATIENT_MANAGEMENT_H
#define PATIENT_MANAGEMENT_H

/*
 * Function: initializePatientSystem
 * --------------------------------
 * Initializes the patient management system by attempting to load existing patient records
 * from patients.dat. 
 */
void initializePatientSystem(void);

/*
 * Function: initializePatientSystemDefault
 * ----------------------------------------
 * Initializes the patient management system with default settings.
 * This function can be used when no custom configuration is needed.
 */
void initializePatientSystemDefault(void);

/*
 * Function: addPatientRecord
 * --------------------------
 * Adds a new patient record to the system.
 */
void addPatientRecord(void);

/*
 * Function: viewPatientRecords
 * ----------------------------
 * Displays all patient records currently stored in the system.
 */
void viewPatientRecords(void);

/*
 * Function: searchPatientById
 * ---------------------------
 * Searches for a patient record using their unique ID.
 */
void searchPatientById(void);

/*
 * Function: dischargePatient
 * --------------------------
 * Removes a patient from the system once they are discharged.
 */
void dischargePatient(void);

void clearMemory();

#endif // PATIENT_MANAGEMENT_H 