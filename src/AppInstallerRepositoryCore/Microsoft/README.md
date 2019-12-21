### Design
The object stack is as such:
- SQLiteIndex :: Houses the database connection and the proper interface with which to interact with it
- ISQLiteIndex :: Interface that creates a uniform model to use against all schemas
- Schema::V*::Interface :: Actual implementation of ISQLiteIndex for specific schema version

The code that needs to interact with an index will create a SQLiteIndex object.  That in turn will open the database, determine the schema version, then create the appropriate ISQLiteIndex providing object.  All queries and changes to the index will go through the SQLiteIndex object.

When a change to the schema is needed, a new schema directory should be created.  This can pull code from the schemas before it, only updating the specific table(s) that are needed.  Then Version code should be updated to create the new Interface as appropriate.  Any new methods needed on ISQLiteIndex should be added, and SQLiteIndexBase to implement them in terms of the older functions.  Only the new schema should need to implement the new functions.   **Once shipped, one should never need to update code within an existing schema, save for bug fixes.**
