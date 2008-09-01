<?php
/**
 * This class has been auto-generated by the Doctrine ORM Framework
 */
class AddObject extends Doctrine_Migration
{
	public function up()
	{
		$this->createTable('object', array (
  'id' => 
  array (
    'primary' => true,
    'autoincrement' => true,
    'type' => 'integer',
    'length' => 4,
  ),
  'name' => 
  array (
    'type' => 'string',
    'length' => 255,
  ),
  'description' => 
  array (
    'type' => 'string',
    'length' => 2147483647,
  ),
  'file_name' => 
  array (
    'type' => 'string',
    'length' => 255,
  ),
  'object_type' => 
  array (
    'type' => 'integer',
    'length' => 4,
  ),
  'downloads' => 
  array (
    'type' => 'integer',
    'length' => 4,
  ),
  'rating' => 
  array (
    'type' => 'float',
    'length' => 2147483647,
  ),
  'approved' => 
  array (
    'type' => 'boolean',
    'length' => 25,
  ),
  'user_id' => 
  array (
    'type' => 'integer',
    'length' => 4,
  ),
  'created_at' => 
  array (
    'type' => 'timestamp',
    'length' => 25,
  ),
  'updated_at' => 
  array (
    'type' => 'timestamp',
    'length' => 25,
  ),
), array (
  'indexes' => 
  array (
  ),
  'primary' => 
  array (
    0 => 'id',
  ),
));
	}

	public function down()
	{
		$this->dropTable('object');
	}
}