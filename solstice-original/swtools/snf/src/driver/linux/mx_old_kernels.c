#if MX_LINUX_NOLOCKPIN == 0

static void mx_activate_page(struct page * page)
{
  /* locked page on some linux versions might jam the inactive_dirty
     list, if we ensure they are active after being locked they should
     never end-up there */
  SetPageReferenced(page);
  mark_page_accessed(page);
}

int
mx_lock_priv_page(struct page *page, mx_page_pin_t *pin, struct vm_area_struct *vma)
{
      
  unsigned count;
  int was_locked = 0;
  int status;
  int count_mx;

  if (!page) {
    /* page might be swapped or read-only, caller should force write
       swapin and try again */
    return EAGAIN;
  }
  pin->private = 1;
  pin->page = page;
  count = page_count(page);
  count_mx = (count >> 16) & 0xfff;

  if (count == 0) {
    MX_WARN (("trying to register a page with count 0\n"));
    return ENXIO;
  }
  
  was_locked = mx_TryLockPage (page);
  /* make sure the page is not already locked by somebody else */  
  if (was_locked && (count_mx == 0)) {
    MX_INFO (("trying to register an already locked page:OK\n"));
    return EAGAIN;
  }
  mx_activate_page(page);

  /* make sure we won't be taking the count too high */
    
  if (count_mx >= 254UL) {
    MX_PRINT(("Not trying to register a page more than 255 times"
	      "    pages=%p , count =0x%x\n",
	      page, count));
    status = ERANGE;
    goto out_with_page_locked;
  }
  
  /* make sure the count hasn't gotten too high without our help.
     use 1 << 15 rather than (1 << 16) - 1 to leave a bit more margin */
  
  if ((count & 0xffff) >= (1UL << 15)) {
    MX_PRINT(("Not trying to register a page with an overly large count"
	      " page=%p\n", page));
    status = ENXIO;
    goto out_with_page_locked;
  }
  
  if (count_mx == 0)
    mx_atomic_subtract(1, &myri_max_user_pinned_pages);
  atomic_add (1UL << 16, &page->mx_p_count);
  return 0;
  
 out_with_page_locked:
  if (!was_locked)
    unlock_page(page);
  return status;
}

void
mx_unlock_priv_page(mx_page_pin_t *pin)
{
  int count;
  int count_mx;
  struct page * page = pin->page;

  count = page_count(page);
  count_mx = (count >> 16) & 0xfff;
  if (count_mx == 1) {
    mx_atomic_add(1, &myri_max_user_pinned_pages);
    unlock_page(page);
  }
  mal_assert (!PageReserved (page));
#if MX_PAGE_COUNT_TO_ZERO
  if (atomic_sub_and_test (1UL << 16, &page->count)) {
    get_page(page);
    page_cache_release(page);
  }
#else
  atomic_sub ((1UL << 16) - 1, &page->mx_p_count);
  page_cache_release(page);
#endif
}

#endif
